from collections import deque

import struct

import pyqtgraph as pg
from PyQt5.QtCore import Qt, pyqtSignal, QSize
from PyQt5.QtGui import QColor, QIntValidator, QPainter
from PyQt5.QtWidgets import (
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QScrollArea,
    QSizePolicy,
    QSlider,
    QVBoxLayout,
    QWidget,
)

from pdm_shared import HISTORY_LEN
from pdm_shared import TRACK_COLORS

from pdm_config_tab import CAN_INPUT_DATA_TYPE_LABELS, IN_MODE_SCHMITT, PAGE_LABEL_STYLE


CONTROL_TOGGLE_STYLE = {
    "on": "#BB86FC",
    "off": "#2E2E2E",
    "knob": "#FFFFFF",
    "border": "#1F1F1F",
}


def _clamp_int(value, minimum, maximum):
    try:
        ivalue = int(value)
    except Exception:
        return minimum
    return max(minimum, min(maximum, ivalue))


def _pack_can_input_value(data_type, value):
    if data_type == 0x00:
        return struct.pack("<B", 1 if bool(value) else 0)
    if data_type == 0x01:
        return struct.pack("<H", _clamp_int(value, 0, 0xFFFF))
    if data_type == 0x02:
        return struct.pack("<I", _clamp_int(value, 0, 0xFFFFFFFF))
    if data_type == 0x03:
        return struct.pack("<h", _clamp_int(value, -0x8000, 0x7FFF))
    if data_type == 0x04:
        return struct.pack("<i", _clamp_int(value, -0x80000000, 0x7FFFFFFF))
    if data_type == 0x05:
        try:
            return struct.pack("<f", float(value))
        except Exception:
            return struct.pack("<f", 0.0)
    return struct.pack("<H", _clamp_int(value, 0, 0xFFFF))


class ToggleSwitch(QWidget):
    toggled = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._checked = False
        self.setCursor(Qt.PointingHandCursor)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        self.setFixedSize(56, 26)

    def isChecked(self):
        return self._checked

    def setChecked(self, checked):
        checked = bool(checked)
        if checked == self._checked:
            self.update()
            return
        self._checked = checked
        self.update()
        self.toggled.emit(self._checked)

    def sizeHint(self):
        return QSize(56, 26)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton and self.isEnabled():
            self.setChecked(not self._checked)
            event.accept()
            return
        super().mouseReleaseEvent(event)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        if self.isEnabled():
            background = QColor(CONTROL_TOGGLE_STYLE["on"] if self._checked else CONTROL_TOGGLE_STYLE["off"])
            knob = QColor(CONTROL_TOGGLE_STYLE["knob"])
        else:
            background = QColor("#3A3A3A")
            knob = QColor("#C0C0C0")

        painter.setPen(QColor(CONTROL_TOGGLE_STYLE["border"]))
        painter.setBrush(background)
        painter.drawRoundedRect(self.rect().adjusted(1, 1, -1, -1), 13, 13)

        diameter = 20
        margin = 3
        x = self.width() - diameter - margin if self._checked else margin
        y = (self.height() - diameter) // 2
        painter.setPen(Qt.NoPen)
        painter.setBrush(knob)
        painter.drawEllipse(x, y, diameter, diameter)


class MiniPreviewChart(QWidget):
    def __init__(self, title, unit, parent=None):
        super().__init__(parent)
        self.title = title
        self.unit = unit
        self._curves = {}
        self._history = {}

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)

        # self.label = QLabel(title)
        # self.label.setStyleSheet("QLabel { color: #BB86FC; font-weight: bold; }")
        # layout.addWidget(self.label)

        self.plot = pg.PlotWidget()
        self.plot.setFixedHeight(92)
        self.plot.setBackground("#1E1E1E")
        self.plot.setMenuEnabled(False)
        self.plot.setMouseEnabled(x=False, y=False)
        self.plot.showGrid(x=False, y=True, alpha=0.15)
        self.plot.hideButtons()
        self.plot.getAxis("left").setPen(pg.mkPen("#777777"))
        self.plot.getAxis("bottom").setPen(pg.mkPen("#777777"))
        self.plot.getAxis("left").setTextPen(pg.mkPen("#BBBBBB"))
        self.plot.getAxis("bottom").setTextPen(pg.mkPen("#BBBBBB"))
        self.plot.setLabel("left", unit)
        self.plot.setLabel("bottom", "")
        layout.addWidget(self.plot)

    def set_series(self, series_items):
        series_items = list(series_items or [])
        if not series_items:
            self.plot.getAxis("bottom").setTicks([[]])
            self.plot.clear()
            self._curves = {}
            self._history = {}
            return

        visible_labels = {str(item.get("label", "")) for item in series_items}
        for item in series_items:
            label = str(item.get("label", ""))
            value = item.get("value", 0)
            if label not in self._history:
                self._history[label] = deque(maxlen=HISTORY_LEN)
            self._history[label].append(value)

        for label in list(self._curves.keys()):
            if label not in visible_labels:
                curve = self._curves.pop(label)
                self.plot.removeItem(curve)
                self._history.pop(label, None)

        self.plot.getAxis("bottom").setTicks([[]])
        max_value = 0
        for item in series_items:
            label = str(item.get("label", ""))
            color = item.get("color", "#BB86FC")
            history = list(self._history.get(label, []))
            x_values = list(range(len(history)))
            if label not in self._curves:
                self._curves[label] = self.plot.plot(
                    [],
                    [],
                    pen=pg.mkPen(color, width=10),
                    name=label,
                )
            else:
                self._curves[label].setPen(pg.mkPen(color, width=2))
                self._curves[label].setSymbolBrush(color)
                self._curves[label].setSymbolPen(color)
            self._curves[label].setData(x_values, history)
            if history:
                max_value = max(max_value, max(history))

        self.plot.enableAutoRange(axis=pg.ViewBox.XYAxes, enable=False)
        self.plot.setYRange(0, max(1, max_value * 1.25))
        self.plot.setXRange(0, max(1, HISTORY_LEN - 1))


class PreviewLegend(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet("QWidget { background-color: #1E1E1E; }")
        self.setMinimumWidth(150)
        self.setMaximumWidth(190)

        self._layout = QVBoxLayout(self)
        self._layout.setContentsMargins(6, 2, 6, 2)
        self._layout.setSpacing(4)
        self._layout.addStretch(1)

    def set_entries(self, entries):
        while self._layout.count():
            item = self._layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()

        for entry in entries or []:
            row = QWidget()
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(0, 0, 0, 0)
            row_layout.setSpacing(6)

            color = str(entry.get("color", "#BB86FC"))
            swatch = QLabel()
            swatch.setFixedSize(10, 10)
            swatch.setStyleSheet(f"QLabel {{ background-color: {color}; border-radius: 5px; }}")
            row_layout.addWidget(swatch)

            label = QLabel(str(entry.get("label", "")))
            label.setStyleSheet("QLabel { color: #E0E0E0; }")
            row_layout.addWidget(label, 1)

            self._layout.addWidget(row)

        self._layout.addStretch(1)


class PhysicalInputRow(QWidget):
    def __init__(self, input_data, parent=None):
        super().__init__(parent)
        self.input_data = dict(input_data or {})
        self.setStyleSheet("QWidget { background-color: #1E1E1E; }")

        layout = QHBoxLayout(self)
        layout.setContentsMargins(8, 2, 8, 2)
        layout.setSpacing(2)

        self.name_label = QLabel(f"PHY {int(self.input_data.get('location', 0)) + 1}")
        self.name_label.setFixedWidth(70)
        self.name_label.setStyleSheet("QLabel { color: #E0E0E0; font-weight: bold; }")
        layout.addWidget(self.name_label)

        self.description_label = QLabel("")
        self.description_label.setFixedWidth(120)
        self.description_label.setStyleSheet("QLabel { color: #B8B8B8; }")
        layout.addWidget(self.description_label)

        state_row = QWidget()
        state_layout = QHBoxLayout(state_row)
        state_layout.setContentsMargins(0, 0, 0, 0)
        state_layout.setSpacing(2)
        state_row.setStyleSheet("QWidget { background-color: #1E1E1E; }")

        self.state_toggle = ToggleSwitch()
        self.state_toggle.setEnabled(False)
        self.state_toggle.setFixedSize(50, 24)
        state_layout.addWidget(self.state_toggle)

        self.state_slider = QSlider(Qt.Horizontal)
        self.state_slider.setRange(0, 5000)
        self.state_slider.setEnabled(False)
        self.state_slider.setFixedHeight(18)
        self.state_slider.setFixedWidth(120)
        self.state_slider.setStyleSheet(
            "QSlider::groove:horizontal { background: #2E2E2E; height: 8px; border-radius: 4px; }"
            "QSlider::sub-page:horizontal { background: #BB86FC; border-radius: 4px; }"
            "QSlider::add-page:horizontal { background: #2E2E2E; border-radius: 4px; }"
            "QSlider::handle:horizontal { background: #FFFFFF; width: 18px; margin: -6px 0; border-radius: 9px; border: 1px solid #7B5FA6; }"
        )
        state_layout.addWidget(self.state_slider, 1)

        self.value_label = QLabel("0 mV")
        self.value_label.setFixedWidth(72)
        self.value_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        state_layout.addWidget(self.value_label)
        layout.addWidget(state_row, 1)

        self._sync_description()
        self.apply_value(0)

    def _sync_description(self):
        mode_label = "Digital" if int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else "Analog"
        self.description_label.setText(mode_label)

    def apply_value(self, value):
        value = max(0, min(5000, int(value)))
        is_digital = int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT
        if is_digital:
            self.state_toggle.setChecked(value >= 2500)
            self.state_toggle.setVisible(True)
            self.state_slider.setVisible(False)
        else:
            self.state_slider.setValue(value)
            self.state_toggle.setVisible(False)
            self.state_slider.setVisible(True)
        self.value_label.setText(f"{value} mV")


class ControlCanInputRow(QGroupBox):
    valueChanged = pyqtSignal(object, object)

    def __init__(self, input_data, parent=None):
        super().__init__(f"CAN input {int(input_data.get('location', 0)) + 1}", parent)
        self.input_data = dict(input_data or {})
        self._updating = False
        self._current_value = 0
        self._usage_labels = []

        self.setStyleSheet(PAGE_LABEL_STYLE + "QGroupBox { background-color: #1E1E1E; }")
        layout = QHBoxLayout(self)
        layout.setContentsMargins(10, 2, 10, 2)
        layout.setSpacing(12)

        left_column = QWidget()
        left_layout = QHBoxLayout(left_column)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(6)

        self.description_label = QLabel("")
        self.description_label.setWordWrap(True)
        left_layout.addWidget(self.description_label)
        left_column.setStyleSheet("QWidget { background-color: #1E1E1E; }")

        control_row = QWidget()
        control_layout = QHBoxLayout(control_row)
        control_layout.setContentsMargins(0, 0, 0, 0)
        control_layout.setSpacing(8)
        control_row.setStyleSheet("QWidget { background-color: #1E1E1E; }")

        self.toggle_switch = ToggleSwitch()
        self.toggle_switch.toggled.connect(self._on_toggle_changed)
        control_layout.addWidget(self.toggle_switch)

        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 5000)
        self.slider.valueChanged.connect(self._on_slider_changed)
        self.slider.setVisible(False)
        self.slider.setFixedHeight(18)
        self.slider.setFixedWidth(120)
        self.slider.setStyleSheet(
            "QSlider::groove:horizontal { background: #2E2E2E; height: 8px; border-radius: 4px; }"
            "QSlider::sub-page:horizontal { background: #BB86FC; border-radius: 4px; }"
            "QSlider::add-page:horizontal { background: #2E2E2E; border-radius: 4px; }"
            "QSlider::handle:horizontal { background: #FFFFFF; width: 18px; margin: -6px 0; border-radius: 9px; border: 1px solid #7B5FA6; }"
        )
        control_layout.addWidget(self.slider)

        self.value_edit = QLineEdit("0")
        self.value_edit.setFixedWidth(96)
        self.value_edit.setValidator(QIntValidator(0, 5000, self))
        self.value_edit.editingFinished.connect(self._on_editing_finished)
        self.value_edit.setVisible(False)
        control_layout.addWidget(self.value_edit)

        self.outputs_label = QLabel("Controlled outputs: none")
        self.outputs_label.setWordWrap(True)
        self.outputs_label.setStyleSheet("QLabel { color: #D8D8D8;}")
        left_layout.addWidget(self.outputs_label)

        control_layout.addStretch(1)
        left_layout.addWidget(control_row)


        layout.addWidget(left_column, 1)

        self.preview_row = QWidget()
        preview_layout = QHBoxLayout(self.preview_row)
        preview_layout.setContentsMargins(0, 0, 0, 0)
        preview_layout.setSpacing(8)
        self.preview_row.setStyleSheet("QWidget { background-color: #1E1E1E; }")
        charts_column = QWidget()
        charts_layout = QHBoxLayout(charts_column)
        charts_layout.setContentsMargins(0, 0, 0, 0)
        charts_layout.setSpacing(8)
        charts_column.setStyleSheet("QWidget { background-color: #1E1E1E; }")
        self.voltage_chart = MiniPreviewChart("Voltage preview", "mV")
        self.current_chart = MiniPreviewChart("Current preview", "mA")
        self.voltage_chart.setMinimumWidth(160)
        self.current_chart.setMinimumWidth(160)
        charts_layout.addWidget(self.voltage_chart)
        charts_layout.addWidget(self.current_chart)
        preview_layout.addWidget(charts_column, 1)
        self.preview_legend = PreviewLegend()
        preview_layout.addWidget(self.preview_legend, 0, Qt.AlignTop)
        layout.addWidget(self.preview_row, 0, Qt.AlignRight | Qt.AlignTop)

        self._sync_description()
        self._sync_mode_widgets()

    def _data_type_label(self, value):
        for label, data_value in CAN_INPUT_DATA_TYPE_LABELS:
            if int(data_value) == int(value):
                return label.replace("CAN_INPUT_TYPE_", "")
        return str(value)

    def _sync_description(self):
        mode_label = "Digital" if int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else "Analog"
        can_instance = int(self.input_data.get("canInstance", 0)) + 1
        can_id = int(self.input_data.get("canId", 0))
        offset = int(self.input_data.get("offset", 0))
        data_type = self._data_type_label(self.input_data.get("dataType", 0))
        self.description_label.setText(f"{mode_label} control, CAN{can_instance}, ID 0x{can_id:03X}, offset {offset}, type {data_type}")

    def _sync_mode_widgets(self):
        is_digital = int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT
        self.toggle_switch.setVisible(is_digital)
        self.slider.setVisible(not is_digital)
        self.value_edit.setVisible(not is_digital)
        self.preview_row.setVisible(False)

    def set_usage_labels(self, usage_labels):
        self._usage_labels = list(usage_labels or [])

    def set_live_channels(self, channels):
        channels = channels or []
        if not self._usage_labels:
            self.outputs_label.setText("Controlled outputs: none")
            self.preview_row.setVisible(False)
            self.preview_legend.set_entries([])
            self.voltage_chart.set_series([])
            self.current_chart.set_series([])
            return

        chip_texts = []
        legend_entries = []
        voltage_series = []
        current_series = []

        for label in self._usage_labels:
            channel_index = None
            channel_name = "OUT"
            if isinstance(label, dict):
                channel_index = label.get("index")
                channel_name = label.get("name") or f"OUT_{int(channel_index) + 1 if channel_index is not None else 0}"
            else:
                channel_name = str(label)

            voltage_text = "- mV"
            channel_voltage = None
            channel_current = None
            color = TRACK_COLORS[0]
            if channel_index is not None:
                try:
                    channel_index = int(channel_index)
                    color = TRACK_COLORS[channel_index % len(TRACK_COLORS)]
                    if 0 <= channel_index < len(channels):
                        channel_voltage = int(channels[channel_index].get("voltage", 0))
                        channel_current = int(channels[channel_index].get("current", 0))
                        voltage_text = f"{channel_voltage} mV"
                except Exception:
                    voltage_text = "- mV"

            if channel_voltage is not None and channel_current is not None:
                voltage_series.append({"label": channel_name, "value": channel_voltage, "color": color})
                current_series.append({"label": channel_name, "value": channel_current, "color": color})
            else:
                voltage_series.append({"label": channel_name, "value": 0, "color": color})
                current_series.append({"label": channel_name, "value": 0, "color": color})
            legend_entries.append({"label": channel_name, "color": color})
            chip_texts.append(f"{channel_name}")

        self.outputs_label.setText("Controlled outputs: " + (" | ".join(chip_texts) if chip_texts else "-"))
        self.preview_legend.set_entries(legend_entries)
        self.preview_row.setVisible(bool(legend_entries))
        self.voltage_chart.set_series(voltage_series)
        self.current_chart.set_series(current_series)

    def set_value(self, value, emit=False):
        is_digital = int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT
        value = int(bool(value)) if is_digital else int(value)
        if value == self._current_value and not emit:
            return

        self._current_value = value
        self._updating = True
        try:
            if is_digital:
                self.toggle_switch.setChecked(bool(value))
            else:
                self.slider.blockSignals(True)
                self.value_edit.blockSignals(True)
                self.slider.setValue(max(0, min(5000, int(value))))
                self.value_edit.setText(str(max(0, min(5000, int(value)))))
                self.slider.blockSignals(False)
                self.value_edit.blockSignals(False)
        finally:
            self._updating = False

        if emit:
            self.valueChanged.emit(self.input_data, self._current_value)

    def current_value(self):
        return self._current_value

    def _on_toggle_changed(self, checked):
        if self._updating:
            return
        self._current_value = 1 if checked else 0
        self.valueChanged.emit(self.input_data, self._current_value)

    def _on_slider_changed(self, value):
        if self._updating:
            return
        self._updating = True
        try:
            self.value_edit.blockSignals(True)
            self.value_edit.setText(str(int(value)))
            self.value_edit.blockSignals(False)
        finally:
            self._updating = False
        self._current_value = int(value)
        self.valueChanged.emit(self.input_data, self._current_value)

    def _on_editing_finished(self):
        if self._updating:
            return
        try:
            value = int(self.value_edit.text())
        except Exception:
            value = self._current_value
        value = max(0, min(5000, value))
        if value == self._current_value:
            self.value_edit.setText(str(value))
            return
        self._updating = True
        try:
            self.slider.blockSignals(True)
            self.slider.setValue(value)
            self.slider.blockSignals(False)
            self.value_edit.setText(str(value))
        finally:
            self._updating = False
        self._current_value = value
        self.valueChanged.emit(self.input_data, value)


class ControlConfigPage(QWidget):
    can_frame_requested = pyqtSignal(int, object)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet(PAGE_LABEL_STYLE)
        self._rows = []
        self._usage_by_input = {}
        self._latest_channels = []
        self._physical_rows = []
        self._latest_physical_values = []

        outer = QVBoxLayout(self)
        outer.setContentsMargins(20, 20, 20, 20)
        outer.setSpacing(8)
        outer.setAlignment(Qt.AlignTop)

        summary = QLabel(
            "Controls are created from enabled CAN inputs. Digital inputs use a large on/off switch; analog inputs use a 0-5000 mV slider and text field."
        )
        summary.setWordWrap(True)
        outer.addWidget(summary)

        self.physical_group = QGroupBox("Physical inputs")
        self.physical_group.setStyleSheet(PAGE_LABEL_STYLE + "QGroupBox { background-color: #1E1E1E; }")
        physical_layout = QVBoxLayout(self.physical_group)
        physical_layout.setContentsMargins(10, 12, 10, 10)
        physical_layout.setSpacing(8)
        self.physical_container = QVBoxLayout()
        self.physical_container.setContentsMargins(0, 0, 0, 0)
        self.physical_container.setSpacing(8)
        physical_layout.addLayout(self.physical_container)
        outer.addWidget(self.physical_group)

        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)
        self.scroll.setFrameShape(QScrollArea.NoFrame)
        self.scroll.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.container = QWidget()
        self.container_layout = QVBoxLayout(self.container)
        self.container_layout.setContentsMargins(0, 0, 0, 0)
        self.container_layout.setSpacing(10)
        self.container_layout.setAlignment(Qt.AlignTop)
        self.scroll.setWidget(self.container)
        outer.addWidget(self.scroll, 1)

        self.empty_label = QLabel("No enabled CAN inputs.")
        self.empty_label.setStyleSheet("QLabel { color: #B0B0B0; font-style: italic; }")
        self.container_layout.addWidget(self.empty_label)

    def _clear_physical_rows(self):
        while self.physical_container.count():
            item = self.physical_container.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()
        self._physical_rows = []

    def set_physical_inputs(self, physical_inputs):
        self._clear_physical_rows()
        for input_data in physical_inputs or []:
            if not isinstance(input_data, dict):
                continue
            row = PhysicalInputRow(input_data)
            self._physical_rows.append(row)
            self.physical_container.addWidget(row)
        if not self._physical_rows:
            placeholder = QLabel("No physical inputs configured.")
            placeholder.setStyleSheet("QLabel { color: #B0B0B0; font-style: italic; }")
            self.physical_container.addWidget(placeholder)
            self._physical_rows = [placeholder]
        self.set_live_physical_values(self._latest_physical_values)

    def set_live_physical_values(self, values):
        self._latest_physical_values = list(values or [])
        for index, row in enumerate(self._physical_rows):
            if hasattr(row, "apply_value"):
                value = self._latest_physical_values[index] if index < len(self._latest_physical_values) else 0
                row.apply_value(value)

    def _clear_rows(self):
        while self.container_layout.count():
            item = self.container_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                if widget is self.empty_label:
                    self.container_layout.removeWidget(widget)
                    widget.setParent(None)
                    widget.hide()
                else:
                    widget.deleteLater()
        self._rows = []

    def refresh_controls(self, can_inputs, usage_by_input=None):
        self._usage_by_input = usage_by_input or {}
        previous_values = {int(row.input_data.get("location", 0)): row.current_value() for row in self._rows}

        self._clear_rows()

        visible_rows = []
        for input_data in can_inputs or []:
            if not isinstance(input_data, dict) or not input_data.get("isUsed"):
                continue

            row_widget = ControlCanInputRow(input_data)
            input_index = int(input_data.get("location", 0))
            source_key = 8 + input_index
            row_widget.set_usage_labels(self._usage_by_input.get(source_key, []))
            if input_index in previous_values:
                row_widget.set_value(previous_values[input_index], emit=False)
            row_widget.valueChanged.connect(self._handle_value_changed)
            self.container_layout.addWidget(row_widget)
            visible_rows.append(row_widget)

        self._rows = visible_rows
        if not self._rows:
            self.container_layout.addWidget(self.empty_label)
            self.empty_label.show()
        else:
            self.empty_label.hide()

        self.set_live_channels(self._latest_channels)

    def set_live_channels(self, channels):
        self._latest_channels = list(channels or [])
        for row in self._rows:
            row.set_live_channels(self._latest_channels)

    def _handle_value_changed(self, input_data, value):
        try:
            arbitration_id = int(input_data.get("canId", 0))
            data_type = int(input_data.get("dataType", 0))
            offset = int(input_data.get("offset", 0))
            payload = bytearray(8)
            encoded_value = _pack_can_input_value(data_type, value)
            if offset < 0 or offset + len(encoded_value) > len(payload):
                raise ValueError(f"Value does not fit into an 8-byte CAN frame at offset {offset}")
            payload[offset : offset + len(encoded_value)] = encoded_value
        except Exception as exc:
            QMessageBox.critical(self, "Control send failed", str(exc))
            return

        self.can_frame_requested.emit(arbitration_id, bytes(payload))
