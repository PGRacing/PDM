import csv
import datetime
import time
from collections import defaultdict, deque

import can
from can import Message

from pdm_shared import (
    CAN_BITRATE,
    CAN_CHANNEL,
    CHANNEL_COUNT,
    IDS,
    PHY_INPUT_COUNT,
    SERIAL_BAUD,
    parse_system_status,
    parse_u16x4,
)


def can_isolated_process(pipe_conn, tx_queue):
    try:
        can_bus = can.interface.Bus(
            channel=CAN_CHANNEL, interface="slcan", bitrate=CAN_BITRATE, ttyBaudrate=SERIAL_BAUD
        )
    except Exception as exc:
        pipe_conn.send({"error": f"Link crash: {exc}"})
        return

    channels = [
        {"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "current_avg": 0}
        for _ in range(CHANNEL_COUNT)
    ]
    phy_inputs = [0] * PHY_INPUT_COUNT
    sys_status = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}
    name_parts = defaultdict(dict)
    current_avg_window = [deque() for _ in range(CHANNEL_COUNT)]

    log_file_name = f"pdmdisp_{datetime.datetime.now():%Y%m%d-%H%M%S}.csv"
    try:
        log_f = open(log_file_name, "a", newline="")
        log_writer = csv.writer(log_f)
        header = ["timestamp", "total_i_mA", "batt_mV", "core_temp_C", "safety"]
        for i in range(CHANNEL_COUNT):
            n = i + 1
            header += [
                f"ch{n}_name",
                f"ch{n}_status",
                f"ch{n}_state",
                f"ch{n}_voltage_mV",
                f"ch{n}_i_inst_mA",
                f"ch{n}_i_avg_mA",
            ]
        for i in range(PHY_INPUT_COUNT):
            header += [f"phy_in{i+1}"]
        log_writer.writerow(header)
    except Exception:
        log_writer = None

    last_gui_update = time.time()
    last_log_time = time.time()
    acquiring = True
    start_time = time.time()
    frame_counts = defaultdict(int)
    frame_first_seen = {}
    frame_last_seen = {}

    while True:
        while not tx_queue.empty():
            task = tx_queue.get_nowait()
            if task.get("cmd") == "EXIT":
                if log_writer:
                    log_f.close()
                return
            if task.get("cmd") == "TOGGLE":
                acquiring = not acquiring
            elif task.get("cmd") == "START":
                acquiring = True
            elif task.get("cmd") == "TX":
                try:
                    cid = task["id"]
                    pld = task["payload"]
                    can_bus.send(Message(arbitration_id=cid, data=pld, is_extended_id=(cid > 0x7FF)))
                except Exception as exc:
                    pipe_conn.send({"error": f"TX err: {exc}"})

        if not acquiring:
            time.sleep(0.02)
            continue

        try:
            msg = can_bus.recv(timeout=0.001)
        except Exception as exc:
            pipe_conn.send({"error": f"Read err: {exc}"})
            continue

        if msg is None:
            continue

        cid = msg.arbitration_id
        d = msg.data
        t_now = time.time()
        frame_counts[cid] += 1
        frame_first_seen.setdefault(cid, t_now)
        frame_last_seen[cid] = t_now

        if cid == IDS["SYS_STATUS"]:
            status, batt_voltage, core_temp, safety_line_state = parse_system_status(d)
            sys_status.update({"status": status, "batt": batt_voltage, "core_temp": core_temp, "safety": safety_line_state})
        elif cid == IDS["STATUS_1_8"]:
            for i in range(min(8, len(d))):
                channels[i]["status"] = d[i]
        elif cid == IDS["STATUS_9_16"]:
            for i in range(min(8, len(d))):
                channels[i + 8]["status"] = d[i]
        elif cid == IDS["STATE_1_16"]:
            for i in range(min(16, len(d) * 2)):
                b = d[i // 2]
                channels[i]["state"] = (b >> 4) & 0x0F if i % 2 == 0 else b & 0x0F
        elif cid in (IDS["VOLT_1_4"], IDS["VOLT_5_8"], IDS["VOLT_9_12"], IDS["VOLT_13_16"]):
            base = {IDS["VOLT_1_4"]: 0, IDS["VOLT_5_8"]: 4, IDS["VOLT_9_12"]: 8, IDS["VOLT_13_16"]: 12}[cid]
            vals = parse_u16x4(d)
            for i in range(4):
                ch = base + i
                if ch < CHANNEL_COUNT:
                    channels[ch]["voltage"] = vals[i]
        elif cid in (IDS["CURR_1_4"], IDS["CURR_5_8"], IDS["CURR_9_12"], IDS["CURR_13_16"]):
            base = {IDS["CURR_1_4"]: 0, IDS["CURR_5_8"]: 4, IDS["CURR_9_12"]: 8, IDS["CURR_13_16"]: 12}[cid]
            vals = parse_u16x4(d)
            for i in range(4):
                ch = base + i
                if ch < CHANNEL_COUNT:
                    inst = vals[i] * 10
                    channels[ch]["current"] = inst
                    win = current_avg_window[ch]
                    win.append((t_now, inst))
                    cutoff = t_now - 0.2
                    while win and win[0][0] < cutoff:
                        win.popleft()
                    channels[ch]["current_avg"] = sum(v for _, v in win) / len(win) if win else 0
            sys_status["total_current"] = sum(ch["current_avg"] for ch in channels)
        elif cid == IDS["NAMES"]:
            meta = d[0]
            part, ch = (meta >> 4) & 0x0F, meta & 0x0F
            if ch < CHANNEL_COUNT:
                name_parts[ch][part] = d[1:].decode(errors="ignore").rstrip("\x00")
                channels[ch]["name"] = "".join(name_parts[ch][i] for i in sorted(name_parts[ch]))
        elif cid == IDS["PHY_INPUTS_1_4"]:
            vals = parse_u16x4(d)
            for i in range(min(4, PHY_INPUT_COUNT)):
                phy_inputs[i] = vals[i]
        elif cid == IDS["PHY_INPUTS_5_8"]:
            vals = parse_u16x4(d)
            for i in range(min(4, PHY_INPUT_COUNT - 4)):
                phy_inputs[i + 4] = vals[i]

        if t_now - last_log_time >= 0.05:
            last_log_time = t_now
            if log_writer:
                try:
                    row = [t_now, f"{sys_status['total_current']:.1f}", sys_status["batt"], f"{sys_status['core_temp']:.1f}", sys_status["safety"]]
                    for ch in channels:
                        row.extend([ch["name"], ch["status"], ch["state"], ch["voltage"], ch["current"], f"{ch['current_avg']:.1f}"])
                    row.extend(phy_inputs)
                    log_writer.writerow(row)
                except Exception:
                    pass

        if t_now - last_gui_update >= 0.03:
            last_gui_update = t_now
            pipe_conn.send(
                {
                    "time": t_now - start_time,
                    "sys": sys_status.copy(),
                    "ch": [c.copy() for c in channels],
                    "phy": list(phy_inputs),
                    "frames": [
                        {
                            "id": fid,
                            "count": frame_counts[fid],
                            "freq": frame_counts[fid] / max(1e-6, frame_last_seen[fid] - frame_first_seen[fid]),
                        }
                        for fid in sorted(frame_counts)
                    ],
                }
            )