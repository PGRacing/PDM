import csv
import datetime
import time
from collections import defaultdict, deque
from pathlib import Path

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

BASE_DIR = Path(__file__).resolve().parent
LOG_DIR = BASE_DIR / "log"
LOG_DIR.mkdir(parents=True, exist_ok=True)

ISOTP_ENTRANCE_ID = 0x450
ISOTP_RX_ID = 0x451
ISOTP_TX_ID = 0x452
ISOTP_GATEWAY_DELAY_S = 0.05
ISOTP_FRAME_DELAY_S = 0.01
ISOTP_GATEWAY_FRAMES = (
    bytes((0xFD, 0xF8, 0x0A, 0xF9, 0x66, 0x56, 0x18, 0x78)),
    bytes((0x80, 0x5A, 0xEF, 0xF0, 0x01, 0xF0, 0x70, 0x48)),
    bytes((0x52, 0xA8, 0xB7, 0x20, 0xAD, 0xAD, 0xFF, 0xF0)),
)

RESET_DEVICE_ID = 0x4AF
RESET_DEVICE_FRAMES = (
    bytes((0xAE, 0x3A, 0x3E, 0x2B, 0x07, 0x17, 0xC8, 0x4C)),
    bytes((0x2D, 0x72, 0x88, 0x04, 0x9F, 0xEA, 0xDA, 0xC7)),
    bytes((0x44, 0x8E, 0x0E, 0xD9, 0x56, 0xA1, 0x35, 0x0F)),
)


def _pipe_progress(pipe_conn, progress, status, done=False, error=None):
    packet = {"isotp_progress": int(progress), "isotp_status": status}
    if done:
        packet["isotp_done"] = True
    if error is not None:
        packet["isotp_error"] = error
    pipe_conn.send(packet)


def _encode_stmin_delay(stmin_value):
    if stmin_value <= 0x7F:
        return stmin_value / 1000.0
    if 0xF1 <= stmin_value <= 0xF9:
        return (stmin_value - 0xF0) / 10000.0
    return 0.0


def _build_isotp_frames(payload):
    payload = bytes(payload)
    if len(payload) <= 7:
        return [bytes([len(payload)]) + payload.ljust(7, b"\0")]

    if len(payload) > 0xFFF:
        raise ValueError("ISO-TP payload too large for classic CAN")

    frames = []
    total_length = len(payload)
    frames.append(bytes([0x10 | ((total_length >> 8) & 0x0F), total_length & 0xFF]) + payload[:6])
    offset = 6
    sequence = 1
    while offset < total_length:
        chunk = payload[offset : offset + 7]
        frames.append(bytes([0x20 | (sequence & 0x0F)]) + chunk.ljust(7, b"\0"))
        sequence = (sequence + 1) & 0x0F
        offset += 7
    return [frame.ljust(8, b"\0") for frame in frames]


def _wait_for_flow_control(can_bus, expected_id, timeout=0.5):
    deadline = time.time() + timeout
    while time.time() < deadline:
        remaining = max(0.0, deadline - time.time())
        msg = can_bus.recv(timeout=remaining)
        if msg is None:
            continue
        if msg.arbitration_id != expected_id:
            continue
        data = bytes(msg.data)
        if not data:
            continue
        if (data[0] >> 4) == 0x3:
            return data
    return None


def _send_isotp_config(can_bus, pipe_conn, payload):
    try:
        total_steps = len(ISOTP_GATEWAY_FRAMES) + len(_build_isotp_frames(payload))
        done_steps = 0

        _pipe_progress(pipe_conn, 0, "Sending gateway frames")
        for index, gateway_frame in enumerate(ISOTP_GATEWAY_FRAMES, start=1):
            can_bus.send(Message(arbitration_id=ISOTP_ENTRANCE_ID, data=gateway_frame, is_extended_id=False))
            done_steps += 1
            percent = int(done_steps * 100 / total_steps)
            _pipe_progress(pipe_conn, percent, f"Gateway frame {index}/3")
            time.sleep(ISOTP_GATEWAY_DELAY_S)

        isotp_frames = _build_isotp_frames(payload)
        if len(isotp_frames) > 1:
            _pipe_progress(pipe_conn, int(done_steps * 100 / total_steps), "Waiting for ISO-TP flow control")
            flow_control = _wait_for_flow_control(can_bus, ISOTP_RX_ID, timeout=0.5)
            stmin_delay = 0.005
            if flow_control is not None:
                flow_status = flow_control[0] & 0x0F
                if flow_status == 0x02:
                    raise RuntimeError("ISO-TP transfer was rejected by the receiver")
                stmin_delay = _encode_stmin_delay(flow_control[2]) or stmin_delay

            for index, iso_frame in enumerate(isotp_frames, start=1):
                can_bus.send(Message(arbitration_id=ISOTP_TX_ID, data=iso_frame, is_extended_id=False))
                done_steps += 1
                percent = int(done_steps * 100 / total_steps)
                _pipe_progress(pipe_conn, percent, f"Frame {index}/{len(isotp_frames)}")
                if index < len(isotp_frames):
                    time.sleep(max(ISOTP_FRAME_DELAY_S, stmin_delay))
        else:
            can_bus.send(Message(arbitration_id=ISOTP_TX_ID, data=isotp_frames[0], is_extended_id=False))
            done_steps += 1
            _pipe_progress(pipe_conn, int(done_steps * 100 / total_steps), "Single frame")

        _pipe_progress(pipe_conn, 100, "Transfer complete", done=True)
    except Exception as exc:
        _pipe_progress(pipe_conn, 0, "Transfer failed", done=True, error=str(exc))


def _send_fixed_frames(can_bus, pipe_conn, arbitration_id, frames, status_prefix="Sending frames"):
    try:
        total_steps = len(frames)
        _pipe_progress(pipe_conn, 0, status_prefix)
        for index, frame_data in enumerate(frames, start=1):
            can_bus.send(Message(arbitration_id=arbitration_id, data=frame_data, is_extended_id=False))
            percent = int(index * 100 / total_steps)
            _pipe_progress(pipe_conn, percent, f"Frame {index}/{total_steps}")
            time.sleep(ISOTP_FRAME_DELAY_S)

        _pipe_progress(pipe_conn, 100, "Reset command sent", done=True)
    except Exception as exc:
        _pipe_progress(pipe_conn, 0, "Reset failed", done=True, error=str(exc))


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

    log_file_name = LOG_DIR / f"pdmdisp_{datetime.datetime.now():%Y%m%d-%H%M%S}.csv"
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
            elif task.get("cmd") == "ISOTP_SEND":
                _send_isotp_config(can_bus, pipe_conn, task["payload"])
            elif task.get("cmd") == "RESET_DEVICE":
                _send_fixed_frames(can_bus, pipe_conn, RESET_DEVICE_ID, RESET_DEVICE_FRAMES, "Sending reset frames")

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