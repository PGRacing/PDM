from collections import deque

import pyqtgraph as pg
from PyQt5.QtWidgets import QCheckBox, QGroupBox, QHBoxLayout, QPushButton, QScrollArea, QVBoxLayout, QWidget

from pdm_shared import CHANNEL_COUNT, HISTORY_LEN, TRACK_COLORS


class Kalman1D:
    def __init__(self, process_noise=0.01, measurement_noise=1.0, initial_value=None):
        self.process_noise = float(process_noise)
        self.measurement_noise = float(measurement_noise)
        self.estimate = float(initial_value) if initial_value is not None else None
        self.estimate_error = 1.0

    def reset(self, value=None):
        self.estimate = float(value) if value is not None else None
        self.estimate_error = 1.0

    def update(self, measurement):
        measurement = float(measurement)
        if self.estimate is None:
            self.estimate = measurement
            return self.estimate

        self.estimate_error += self.process_noise
        kalman_gain = self.estimate_error / (self.estimate_error + self.measurement_noise)
        self.estimate = self.estimate + kalman_gain * (measurement - self.estimate)
        self.estimate_error = (1.0 - kalman_gain) * self.estimate_error
        return self.estimate


class PlotPanel(QWidget):
    MIN_VOLTAGE_RANGE_MV = 12000
    MIN_CURRENT_RANGE_MA = 3000
    VOLTAGE_HEADROOM_MV = 2000
    CURRENT_HEADROOM_MA = 1000

    def __init__(self, parent=None):
        super().__init__(parent)

        self.hist_v = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_i = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_iavg = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_v_filt = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_i_filt = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_sys_v = deque(maxlen=HISTORY_LEN)
        self.hist_sys_i = deque(maxlen=HISTORY_LEN)
        self.hist_sys_v_filt = deque(maxlen=HISTORY_LEN)
        self.hist_sys_i_filt = deque(maxlen=HISTORY_LEN)

        self.kf_v = [Kalman1D(process_noise=0.03, measurement_noise=0.2) for _ in range(CHANNEL_COUNT)]
        self.kf_i = [Kalman1D(process_noise=0.10, measurement_noise=0.2) for _ in range(CHANNEL_COUNT)]
        self.kf_sys_v = Kalman1D(process_noise=0.03, measurement_noise=0.2)
        self.kf_sys_i = Kalman1D(process_noise=0.10, measurement_noise=0.2)

        self.curves_v = {}
        self.curves_i_inst = {}
        self.curves_i_avg = {}
        self.curves_sys_v = None
        self.curves_sys_i = None

        root_layout = QHBoxLayout(self)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(12)

        ch_box = QGroupBox("Output channel:")
        ch_box.setMinimumWidth(200)
        # ch_box.setMaximumWidth(260)
        ch_layout = QVBoxLayout(ch_box)
        ch_layout.setContentsMargins(10, 12, 10, 10)
        ch_layout.setSpacing(2)
        self.channel_checkboxes = []
        self.channel_scroll = QScrollArea()
        self.channel_scroll.setWidgetResizable(False)
        self.channel_scroll.setFrameShape(QScrollArea.NoFrame)
        channel_content = QWidget()
        channel_layout = QVBoxLayout(channel_content)
        channel_layout.setContentsMargins(0, 0, 0, 0)
        channel_layout.setSpacing(1)
        channel_layout.addStretch(2)
        channel_content.setMinimumWidth(150)
        for i in range(CHANNEL_COUNT):
            cb = QCheckBox(f"CH{i+1}")
            cb.setChecked(i == 0)
            cb.setStyleSheet("QCheckBox { padding: 6px 8px; margin: 2px 0; }")
            cb.stateChanged.connect(self.refresh_plots)
            self.channel_checkboxes.append(cb)
            channel_layout.insertWidget(channel_layout.count() - 1, cb)
        self.system_checkbox = QCheckBox("System")
        self.system_checkbox.setChecked(True)
        self.system_checkbox.setStyleSheet("QCheckBox { padding: 6px 8px; margin: 2px 0; }")
        self.system_checkbox.stateChanged.connect(self.refresh_plots)
        channel_layout.insertWidget(channel_layout.count() - 1, self.system_checkbox)
        self.channel_scroll.setWidget(channel_content)
        ch_layout.addWidget(self.channel_scroll, 1)

        button_row = QVBoxLayout()
        button_row.setSpacing(6)
        self.btn_show_all = QPushButton("Show all")
        self.btn_show_hp = QPushButton("Show HP")
        self.btn_show_lp = QPushButton("Show LP")
        self.btn_show_all.clicked.connect(self.toggle_show_all)
        self.btn_show_hp.clicked.connect(self.toggle_show_hp)
        self.btn_show_lp.clicked.connect(self.toggle_show_lp)
        button_row.addWidget(self.btn_show_all)
        button_row.addWidget(self.btn_show_hp)
        button_row.addWidget(self.btn_show_lp)
        ch_layout.addLayout(button_row)
        root_layout.addWidget(ch_box, stretch=0)

        plots_panel = QWidget()
        plots_layout = QVBoxLayout(plots_panel)
        plots_layout.setContentsMargins(0, 0, 0, 0)
        plots_layout.setSpacing(0)

        self.graph_container = pg.GraphicsLayoutWidget()
        self.graph_container.setBackground("#1E1E1E")
        plots_layout.addWidget(self.graph_container)
        self.graph_container.setToolTip("Click Ctrl + Space to pause the charts")
        root_layout.addWidget(plots_panel, stretch=3)

        self.plot_v = self.graph_container.addPlot(row=0, col=0)
        self.plot_i = self.graph_container.addPlot(row=1, col=0)

        self.legend_v = self.plot_v.addLegend(offset=(10, 10), colCount=2)
        self.legend_i = self.plot_i.addLegend(offset=(10, 10), colCount=2)

        self.plot_v.setTitle("Voltage", color="#BB86FC", size="11pt")
        self.plot_v.setLabel("left", "Voltage", units="mV")
        self.plot_v.showGrid(x=True, y=True, alpha=0.15)
        self.plot_v.setMouseEnabled(x=False, y=False)
        self.plot_v.getViewBox().setMouseEnabled(x=False, y=False)
        self.plot_v.setMenuEnabled(False)

        self.plot_i.setTitle("Current", color="#BB86FC", size="11pt")
        self.plot_i.setLabel("left", "Current", units="mA")
        self.plot_i.showGrid(x=True, y=True, alpha=0.15)
        self.plot_i.setMouseEnabled(x=False, y=False)
        self.plot_i.getViewBox().setMouseEnabled(x=False, y=False)
        self.plot_i.setMenuEnabled(False)

        for ch in range(CHANNEL_COUNT):
            clr = TRACK_COLORS[ch % len(TRACK_COLORS)]
            self.curves_v[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr, width=1.5), name=f"CH{ch+1}")
            self.plot_v.addItem(self.curves_v[ch])
            self.curves_i_inst[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr, width=1.5), name=f"CH{ch+1} I")
            self.plot_i.addItem(self.curves_i_inst[ch])
            self.curves_i_avg[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr + "40", width=1), name=f"CH{ch+1} Irms")
            self.plot_i.addItem(self.curves_i_avg[ch])

        sys_pen = pg.mkPen(color="#F2C94C", width=2)
        self.curves_sys_v = pg.PlotDataItem(pen=sys_pen, name="System")
        self.plot_v.addItem(self.curves_sys_v)
        self.curves_sys_i = pg.PlotDataItem(pen=sys_pen, name="System")
        self.plot_i.addItem(self.curves_sys_i)

        self._refresh_legends()
        self._apply_minimum_axis_ranges()

    def update_from_packet(self, packet):
        t_now = packet["time"]
        for i, ch in enumerate(packet["ch"]):
            filtered_voltage = self.kf_v[i].update(ch["voltage"])
            filtered_current = self.kf_i[i].update(ch["current"])

            self.hist_v[i].append((t_now, ch["voltage"]))
            self.hist_i[i].append((t_now, ch["current"]))
            self.hist_iavg[i].append((t_now, ch["current_avg"]))
            self.hist_v_filt[i].append((t_now, filtered_voltage))
            self.hist_i_filt[i].append((t_now, filtered_current))

        sys_data = packet.get("sys", {}) or {}
        filtered_sys_voltage = self.kf_sys_v.update(sys_data.get("batt", 0))
        filtered_sys_current = self.kf_sys_i.update(sys_data.get("total_current", 0))
        self.hist_sys_v.append((t_now, sys_data.get("batt", 0)))
        self.hist_sys_i.append((t_now, sys_data.get("total_current", 0)))
        self.hist_sys_v_filt.append((t_now, filtered_sys_voltage))
        self.hist_sys_i_filt.append((t_now, filtered_sys_current))

    def apply_channel_names(self, channel_names):
        for i, name in enumerate(channel_names):
            display_name = f"CH{(i+1):<6} {name.strip()}" if name.strip() else f"CH{i+1}"
            if self.channel_checkboxes[i].text() != display_name:
                self.channel_checkboxes[i].setText(display_name)

    def refresh_plots(self):
        selected_channels = [i for i, cb in enumerate(self.channel_checkboxes) if cb.isChecked()]

        for ch in range(CHANNEL_COUNT):
            if ch in selected_channels:
                if len(self.hist_v[ch]) > 1:
                    t, v = zip(*self.hist_v_filt[ch])
                    self.curves_v[ch].setData(t, v)
                else:
                    self.curves_v[ch].setData([], [])

                if len(self.hist_i[ch]) > 1:
                    t, c = zip(*self.hist_i_filt[ch])
                    self.curves_i_inst[ch].setData(t, c)
                else:
                    self.curves_i_inst[ch].setData([], [])

                if len(self.hist_iavg[ch]) > 1:
                    t, c = zip(*self.hist_iavg[ch])
                    self.curves_i_avg[ch].setData(t, c)
                else:
                    self.curves_i_avg[ch].setData([], [])
            else:
                self.curves_v[ch].setData([], [])
                self.curves_i_inst[ch].setData([], [])
                self.curves_i_avg[ch].setData([], [])

        if self.system_checkbox.isChecked() and len(self.hist_sys_v_filt) > 1:
            t, v = zip(*self.hist_sys_v_filt)
            self.curves_sys_v.setData(t, v)
        else:
            self.curves_sys_v.setData([], [])

        if self.system_checkbox.isChecked() and len(self.hist_sys_i_filt) > 1:
            t, i = zip(*self.hist_sys_i_filt)
            self.curves_sys_i.setData(t, i)
        else:
            self.curves_sys_i.setData([], [])

        self._refresh_legends(selected_channels)
        self._apply_minimum_axis_ranges(selected_channels)

    def _refresh_legends(self, selected_channels=None):
        selected_channels = [i for i, cb in enumerate(self.channel_checkboxes) if cb.isChecked()] if selected_channels is None else list(selected_channels)

        self.legend_v.clear()
        self.legend_i.clear()

        for ch in selected_channels:
            self.legend_v.addItem(self.curves_v[ch], f"CH{ch + 1}")
            self.legend_i.addItem(self.curves_i_inst[ch], f"CH{ch + 1} I")
            self.legend_i.addItem(self.curves_i_avg[ch], f"CH{ch + 1} Irms")

        if self.system_checkbox.isChecked():
            self.legend_v.addItem(self.curves_sys_v, "System")
            self.legend_i.addItem(self.curves_sys_i, "System")

    def _apply_minimum_axis_ranges(self, selected_channels=None):
        selected_channels = list(range(CHANNEL_COUNT)) if selected_channels is None else list(selected_channels)

        voltage_values = [value for ch in selected_channels for _, value in self.hist_v_filt[ch]]
        current_values = [value for ch in selected_channels for _, value in self.hist_i_filt[ch]]
        current_values.extend(value for ch in selected_channels for _, value in self.hist_iavg[ch])

        if self.system_checkbox.isChecked():
            voltage_values.extend(value for _, value in self.hist_sys_v_filt)
            current_values.extend(value for _, value in self.hist_sys_i_filt)

        self._set_minimum_range(
            self.plot_v,
            voltage_values,
            self.MIN_VOLTAGE_RANGE_MV,
            force_zero_lower=True,
            top_margin=self.VOLTAGE_HEADROOM_MV,
        )
        self._set_minimum_range(
            self.plot_i,
            current_values,
            self.MIN_CURRENT_RANGE_MA,
            force_zero_lower=True,
            top_margin=self.CURRENT_HEADROOM_MA,
        )

    @staticmethod
    def _set_minimum_range(plot_item, values, minimum_range, force_zero_lower=False, top_margin=0):
        if values:
            lower = 0 if force_zero_lower else max(0, min(values))
            upper = max(max(values) + top_margin, lower + minimum_range)
        else:
            lower = 0
            upper = minimum_range + top_margin
        plot_item.setYRange(lower, upper, padding=0)

    def _set_channels(self, indices_to_show):
        visible_indices = set(indices_to_show)
        for i, cb in enumerate(self.channel_checkboxes):
            cb.setChecked(i in visible_indices)

    def _all_checked(self, start_index, end_index):
        return all(self.channel_checkboxes[i].isChecked() for i in range(start_index, end_index))

    def _none_checked(self):
        return not any(cb.isChecked() for cb in self.channel_checkboxes)

    def toggle_show_all(self):
        if all(cb.isChecked() for cb in self.channel_checkboxes):
            self._set_channels([])
            self.system_checkbox.setChecked(False)
        else:
            self._set_channels(range(CHANNEL_COUNT))
            self.system_checkbox.setChecked(True)

    def toggle_show_hp(self):
        hp_range = range(0, 8)
        if self._all_checked(0, 8) and all(not self.channel_checkboxes[i].isChecked() for i in range(8, CHANNEL_COUNT)):
            self._set_channels([])
        else:
            self._set_channels(hp_range)

    def toggle_show_lp(self):
        lp_range = range(8, 16)
        if self._all_checked(8, CHANNEL_COUNT) and all(not self.channel_checkboxes[i].isChecked() for i in range(0, 8)):
            self._set_channels([])
        else:
            self._set_channels(lp_range)