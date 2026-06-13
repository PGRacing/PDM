from collections import deque
import struct

from PyQt5.QtCore import QPointF, Qt, QSize, pyqtSignal
from PyQt5.QtGui import QColor, QPainter, QPen, QPolygonF
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QSizePolicy,
    QSlider,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
    QHeaderView,
)

from pdm_shared import HISTORY_LEN, TRACK_COLORS
from pdm_config_tab import IN_MODE_SCHMITT, PAGE_LABEL_STYLE


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
        self.set_live_channels(self._latest_channels)

    def _create_row(self, index, input_data, kind, source_id, previous_value):
        editable = kind == "can"
        control = ControlValueCell(input_data, editable)
        if editable:
            control.valueChanged.connect(self._handle_value_changed)
            if previous_value is not None:
                control.set_value(previous_value, emit=False)
        voltage_chart = SparklineWidget("mV")
        current_chart = SparklineWidget("mA")
        outputs = list(self._usage_by_input.get(source_id, []))
        print(outputs)
        return {
            "kind": kind,
            "index": index,
            "source_id": source_id,
            "input_data": dict(input_data or {}),
            "outputs": outputs,
            "control": control,
            "voltage_chart": voltage_chart,
            "current_chart": current_chart,
        }

    def _apply_row(self, row_index, row):
        input_data = row["input_data"]
        kind = row["kind"]
        index = row["index"]

        if kind == "physical":
            input_name = f"Physical input {index + 1}"
            input_source = f"PHY{index + 1}"
        else:
            can_instance = int(input_data.get("canInstance", 0)) + 1
            can_id = int(input_data.get("canId", 0))
            input_name = f"CAN input {index + 1}"
            input_source = f"CAN{can_instance} 0x{can_id:03X}"

        input_type = "Digital" if int(input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else "Analog"

        self._set_item(row_index, 0, input_name)
        self._set_item(row_index, 1, input_source)
        self._set_item(row_index, 2, input_type)
        self._set_item(row_index, 3, self._format_outputs(row["outputs"]))
        self.table.setCellWidget(row_index, 4, row["control"])
        self.table.setCellWidget(row_index, 5, row["voltage_chart"])
        self.table.setCellWidget(row_index, 6, row["current_chart"])

    def _set_item(self, row, column, text):
        item = QTableWidgetItem(str(text))
        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
        item.setToolTip(str(text))
        self.table.setItem(row, column, item)

    def _format_outputs(self, outputs):
        labels = []
        for entry in outputs or []:
            if isinstance(entry, dict):
                label = entry.get("name")
                if not label:
                    index = entry.get("index")
                    label = f"OUT_{int(index) + 1}" if index is not None else "OUT"
                labels.append(str(label))
            else:
                labels.append(str(entry))
        return ", ".join(labels) if labels else "-"

    def _handle_value_changed(self, input_data, value):
        try:
            arbitration_id = int(input_data.get("canId", 0))
            data_type = int(input_data.get("dataType", 0))
            offset = int(input_data.get("offset", 0))
            payload = bytearray(8)
            encoded_value = _pack_can_input_value(data_type, value)
            if offset < 0 or offset + len(encoded_value) > len(payload):
                raise ValueError("Value does not fit into an 8-byte CAN frame at the selected offset")
            payload[offset : offset + len(encoded_value)] = encoded_value
        except Exception as exc:
            QMessageBox.critical(self, "Control send failed", str(exc))
            return

        self.can_frame_requested.emit(arbitration_id, bytes(payload))
from collections import deque
import struct

from PyQt5.QtCore import QPointF, Qt, QSize, pyqtSignal
from PyQt5.QtGui import QColor, QPainter, QPen, QPolygonF
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QHeaderView,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QSizePolicy,
    QSlider,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from pdm_shared import HISTORY_LEN, TRACK_COLORS
from pdm_config_tab import IN_MODE_SCHMITT, PAGE_LABEL_STYLE


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


def _scale_slider_to_uint32(value):
    try:
        slider_value = max(0, min(5000, int(value)))
    except Exception:
        slider_value = 0
    return int(round((slider_value / 5000.0) * 0xFFFFFFFF))


def _scale_slider_to_range(value, minimum, maximum):
    try:
        slider_value = max(0, min(5000, int(value)))
    except Exception:
        slider_value = 0

    if maximum <= minimum:
        return int(minimum)

    ratio = slider_value / 5000.0
    return int(round(minimum + (ratio * (maximum - minimum))))


def _encode_can_slider_value(data_type, value):
    if data_type == 0x00:
        return struct.pack("<B", 1 if bool(value) else 0)
    if data_type == 0x01:
        return struct.pack("<H", _scale_slider_to_range(value, 0, 0xFFFF))
    if data_type == 0x02:
        return struct.pack("<I", _scale_slider_to_range(value, 0, 0xFFFFFFFF))
    if data_type == 0x03:
        return struct.pack("<h", _scale_slider_to_range(value, -0x8000, 0x7FFF))
    if data_type == 0x04:
        return struct.pack("<i", _scale_slider_to_range(value, -0x80000000, 0x7FFFFFFF))
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


class SparklineWidget(QWidget):
    def __init__(self, unit, parent=None):
        super().__init__(parent)
        self.unit = unit
        self._series = {}
        self._colors = {}
        self.setMinimumHeight(56)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

    def clear(self):
        self._series.clear()
        self._colors.clear()
        self.update()

    def set_series(self, series_items):
        series_items = list(series_items or [])
        visible_labels = set()

        for item in series_items:
            label = str(item.get("label", "")).strip()
            if not label:
                continue
            visible_labels.add(label)
            try:
                value = float(item.get("value", 0))
            except Exception:
                value = 0.0
            history = self._series.setdefault(label, deque(maxlen=HISTORY_LEN))
            history.append(value)
            self._colors[label] = QColor(str(item.get("color", "#BB86FC")))

        for label in list(self._series.keys()):
            if label not in visible_labels:
                self._series.pop(label, None)
                self._colors.pop(label, None)

        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        rect = self.rect().adjusted(1, 1, -1, -1)
        painter.fillRect(rect, QColor("#1E1E1E"))
        painter.setPen(QPen(QColor("#333333"), 1))
        painter.drawRect(rect)

        plot_rect = rect.adjusted(6, 6, -6, -6)
        if plot_rect.width() <= 0 or plot_rect.height() <= 0:
            return

        values = []
        for history in self._series.values():
            values.extend(history)
        if not values:
            painter.setPen(QColor("#8A8A8A"))
            painter.drawText(plot_rect, Qt.AlignCenter, "No data")
            return

        min_value = min(values)
        max_value = max(values)
        if min_value == max_value:
            max_value = min_value + 1.0

        painter.setPen(QPen(QColor("#2A2A2A"), 1))
        for index in range(1, 4):
            y = plot_rect.top() + (plot_rect.height() * index) / 4.0
            painter.drawLine(plot_rect.left(), int(y), plot_rect.right(), int(y))

        for label, history in self._series.items():
            color = self._colors.get(label, QColor("#BB86FC"))
            painter.setPen(QPen(color, 2))
            if len(history) == 1:
                x = plot_rect.center().x()
                y = self._value_to_y(history[0], min_value, max_value, plot_rect)
                painter.drawEllipse(QPointF(x, y), 2.5, 2.5)
                continue

            points = []
            max_count = max(1, HISTORY_LEN - 1)
            for index, value in enumerate(history):
                x = plot_rect.left() + (plot_rect.width() * index) / max_count
                y = self._value_to_y(value, min_value, max_value, plot_rect)
                points.append(QPointF(float(x), float(y)))

            if len(points) >= 2:
                painter.drawPolyline(QPolygonF(points))

        if self.unit:
            painter.setPen(QColor("#B8B8B8"))
            painter.drawText(rect.adjusted(8, 2, -8, -2), Qt.AlignTop | Qt.AlignRight, self.unit)

    @staticmethod
    def _value_to_y(value, minimum, maximum, rect):
        ratio = (float(value) - float(minimum)) / max(1e-9, float(maximum) - float(minimum))
        ratio = max(0.0, min(1.0, ratio))
        return rect.bottom() - (ratio * rect.height())


class ControlValueCell(QWidget):
    valueChanged = pyqtSignal(object, object)

    def __init__(self, input_data, editable, parent=None):
        super().__init__(parent)
        self.input_data = dict(input_data or {})
        self._editable = bool(editable)
        self._is_digital = int(self.input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT
        self._updating = False
        self._current_value = 0

        self.setStyleSheet("QWidget { background-color: #1E1E1E; }")
        layout = QHBoxLayout(self)
        layout.setContentsMargins(3, 1, 3, 1)
        layout.setSpacing(3)

        self.value_label = QLabel("-")
        self.value_label.setStyleSheet("QLabel { color: #E0E0E0; }")

        if not self._editable:
            if self._is_digital:
                self.toggle = ToggleSwitch()
                self.toggle.setEnabled(False)
                self.toggle.setFocusPolicy(Qt.NoFocus)
                layout.addWidget(self.toggle)
            else:
                self.slider = QSlider(Qt.Horizontal)
                self.slider.setRange(0, 5000)
                self.slider.setEnabled(False)
                self.slider.setFocusPolicy(Qt.NoFocus)
                self.slider.setFixedHeight(20)
                self.slider.setStyleSheet(
                    "QSlider::groove:horizontal { background: #2E2E2E; height: 8px; border-radius: 4px; }"
                    "QSlider::sub-page:horizontal { background: #BB86FC; border-radius: 4px; }"
                    "QSlider::add-page:horizontal { background: #2E2E2E; border-radius: 4px; }"
                    "QSlider::handle:horizontal { background: #FFFFFF; width: 18px; margin: -6px 0; border-radius: 9px; border: 1px solid #7B5FA6; }"
                )
                layout.addWidget(self.slider, 1)
            layout.addStretch(1)
            self.set_value(0, emit=False)
            return

        if self._is_digital:
            self.toggle = ToggleSwitch()
            self.toggle.setEnabled(True)
            self.toggle.setFocusPolicy(Qt.NoFocus)
            layout.addWidget(self.toggle)
        else:
            self.slider = QSlider(Qt.Horizontal)
            self.slider.setRange(0, 5000)
            self.slider.setEnabled(True)
            self.slider.setFocusPolicy(Qt.NoFocus)
            self.slider.setTracking(True)
            self.slider.setSingleStep(1)
            self.slider.setPageStep(1)
            self.slider.valueChanged.connect(self._on_slider_changed)
            self.slider.sliderMoved.connect(self._on_slider_changed)
            self.slider.sliderReleased.connect(self._on_slider_released)
            self.slider.setFixedHeight(20)
            self.slider.setStyleSheet(
                "QSlider::groove:horizontal { background: #2E2E2E; height: 8px; border-radius: 4px; }"
                "QSlider::sub-page:horizontal { background: #BB86FC; border-radius: 4px; }"
                "QSlider::add-page:horizontal { background: #2E2E2E; border-radius: 4px; }"
                "QSlider::handle:horizontal { background: #FFFFFF; width: 18px; margin: -6px 0; border-radius: 9px; border: 1px solid #7B5FA6; }"
            )
            layout.addWidget(self.slider, 1)

        if self._editable:
            if self._is_digital:
                self.toggle.toggled.connect(self._on_toggle_changed)
            else:
                self.value_label = QLabel("0 mV")
                self.value_label.setStyleSheet("QLabel { color: #E0E0E0; }")
                self.value_label.setFixedWidth(82)
                self.value_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
                layout.addWidget(self.value_label)

        layout.addStretch(1)
        self.set_value(0, emit=False)

    def current_value(self):
        return self._current_value

    def set_value(self, value, emit=False):
        if self._is_digital:
            value = 1 if bool(value) else 0
        else:
            try:
                value = max(0, min(5000, int(value)))
            except Exception:
                value = 0

        if value == self._current_value and not emit:
            self._update_display(value)
            return

        self._current_value = value
        self._updating = True
        try:
            if self._editable and self._is_digital:
                self.toggle.blockSignals(True)
                self.toggle.setChecked(bool(value))
                self.toggle.blockSignals(False)
            elif self._editable:
                self.slider.blockSignals(True)
                self.slider.setValue(value)
                self.slider.blockSignals(False)
        finally:
            self._updating = False

        self._update_display(value)
        if emit:
            self.valueChanged.emit(self.input_data, self._current_value)

    def set_display_value(self, value):
        try:
            value = int(value)
        except Exception:
            value = 0
        self._current_value = value if not self._is_digital else (1 if value >= 2500 else 0)
        self._update_display(value)

    def _update_display(self, value):
        if self._is_digital:
            try:
                mvolts = int(value)
            except Exception:
                mvolts = 0
            if self._editable:
                self.toggle.blockSignals(True)
                self.toggle.setChecked(bool(value))
                self.toggle.blockSignals(False)
            else:
                self.toggle.blockSignals(True)
                self.toggle.setChecked(mvolts >= 2750)
                self.toggle.blockSignals(False)
        else:
            try:
                mvolts = int(value)
            except Exception:
                mvolts = 0
            if self._editable:
                self.value_label.setText(f"{mvolts} mV")
            else:
                self.slider.blockSignals(True)
                self.slider.setValue(mvolts)
                self.slider.blockSignals(False)

    def _on_toggle_changed(self, checked):
        if self._updating:
            return
        self._current_value = 1 if checked else 0
        self.valueChanged.emit(self.input_data, self._current_value)

    def _on_slider_changed(self, value):
        if self._updating:
            return
        self._current_value = int(value)
        self._update_display(self._current_value)
        self.valueChanged.emit(self.input_data, self._current_value)

    def _on_slider_released(self):
        if self._updating:
            return
        self.valueChanged.emit(self.input_data, self._current_value)


class ControlConfigPage(QWidget):
    can_frame_requested = pyqtSignal(int, object)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet(PAGE_LABEL_STYLE)
        self._physical_inputs = []
        self._can_inputs = []
        self._usage_by_input = {}
        self._latest_channels = []
        self._latest_physical_values = []
        self._rows = []

        outer = QVBoxLayout(self)
        outer.setContentsMargins(20, 20, 20, 20)
        outer.setSpacing(4)

        summary = QLabel(
            "One row per input. Physical rows are display-only. CAN rows expose a toggle for digital inputs or a slider for analog inputs."
        )
        summary.setWordWrap(True)
        outer.addWidget(summary)

        self.table = QTableWidget(0, 7)
        self.table.setHorizontalHeaderLabels([
            "Input name",
            "Input source",
            "Input type",
            "Controlled output list",
            "Control",
            "Controlled channels voltage chart",
            "Controlled channels current chart",
        ])
        self.table.setAlternatingRowColors(True)
        self.table.setShowGrid(True)
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table.setSelectionMode(QAbstractItemView.NoSelection)
        self.table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.table.setWordWrap(True)
        self.table.verticalHeader().setVisible(False)
        self.table.verticalHeader().setDefaultSectionSize(68)
        self.table.verticalHeader().setMinimumSectionSize(60)
        self.table.horizontalHeader().setStretchLastSection(False)
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeToContents)
        self.table.horizontalHeader().setSectionResizeMode(3, QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(4, QHeaderView.Interactive)
        self.table.horizontalHeader().setSectionResizeMode(5, QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(6, QHeaderView.Stretch)
        self.table.setColumnWidth(4, 260)
        self.table.setStyleSheet("QTableWidget::item { padding: 1px 3px; }")
        outer.addWidget(self.table, 1)

    def set_physical_inputs(self, physical_inputs):
        self._physical_inputs = [dict(item) for item in physical_inputs or [] if isinstance(item, dict)]
        self._rebuild_table()

    def refresh_controls(self, can_inputs, usage_by_input=None):
        self._can_inputs = [dict(item) for item in can_inputs or [] if isinstance(item, dict)]
        self._usage_by_input = dict(usage_by_input or {})
        self._rebuild_table()

    def set_live_physical_values(self, values):
        self._latest_physical_values = list(values or [])
        for row in self._rows:
            if row["kind"] != "physical":
                continue
            source_id = row["source_id"]
            value = self._latest_physical_values[source_id] if source_id < len(self._latest_physical_values) else 0
            row["control"].set_display_value(value)

    def set_live_channels(self, channels):
        self._latest_channels = list(channels or [])
        for row in self._rows:
            outputs = row["outputs"]
            if not outputs:
                row["voltage_chart"].clear()
                row["current_chart"].clear()
                continue

            voltage_series = []
            current_series = []
            for output in outputs:
                if not isinstance(output, dict):
                    continue
                channel_index = output.get("index")
                if channel_index is None:
                    continue
                try:
                    channel_index = int(channel_index)
                except Exception:
                    continue
                if channel_index < 0 or channel_index >= len(self._latest_channels):
                    continue

                channel = self._latest_channels[channel_index]
                channel_name = str(output.get("name") or channel.get("name") or f"OUT_{channel_index + 1}")
                color = TRACK_COLORS[channel_index % len(TRACK_COLORS)]
                try:
                    voltage = int(channel.get("voltage", 0))
                except Exception:
                    voltage = 0
                try:
                    current = int(channel.get("current", 0))
                except Exception:
                    current = 0

                voltage_series.append({"label": channel_name, "value": voltage, "color": color})
                current_series.append({"label": channel_name, "value": current, "color": color})

            row["voltage_chart"].set_series(voltage_series)
            row["current_chart"].set_series(current_series)

    def _clear_table(self):
        for row_index in range(self.table.rowCount()):
            for column_index in range(self.table.columnCount()):
                widget = self.table.cellWidget(row_index, column_index)
                if widget is not None:
                    widget.deleteLater()
        self.table.clearContents()
        self.table.setRowCount(0)
        self._rows = []

    def _rebuild_table(self):
        previous_values = {row["source_id"]: row["control"].current_value() for row in self._rows if row["kind"] == "can"}

        self._clear_table()
        rows = []

        for index, input_data in enumerate(self._physical_inputs):
            source_id = int(input_data.get("location", index))
            rows.append(self._create_row(index, input_data, "physical", source_id, None))

        for index, input_data in enumerate(self._can_inputs):
            if not input_data.get("isUsed"):
                continue
            source_id = 8 + int(input_data.get("location", index))
            rows.append(self._create_row(index, input_data, "can", source_id, previous_values.get(source_id)))

        self._rows = rows
        self.table.setRowCount(len(rows))
        for row_index, row in enumerate(rows):
            self.table.setRowHeight(row_index, 68)
            self._apply_row(row_index, row)

        self.set_live_physical_values(self._latest_physical_values)
        self.set_live_channels(self._latest_channels)

    def _create_row(self, index, input_data, kind, source_id, previous_value):
        editable = kind == "can"
        control = ControlValueCell(input_data, editable)
        if editable:
            control.valueChanged.connect(self._handle_value_changed)
            if previous_value is not None:
                control.set_value(previous_value, emit=False)
        else:
            control.set_display_value(self._latest_physical_values[source_id] if source_id < len(self._latest_physical_values) else 0)
        voltage_chart = SparklineWidget("mV")
        current_chart = SparklineWidget("mA")
        outputs = list(self._usage_by_input.get(source_id, []))
        return {
            "kind": kind,
            "index": index,
            "source_id": source_id,
            "input_data": dict(input_data or {}),
            "outputs": outputs,
            "control": control,
            "voltage_chart": voltage_chart,
            "current_chart": current_chart,
        }

    def _apply_row(self, row_index, row):
        input_data = row["input_data"]
        kind = row["kind"]
        index = row["index"]

        if kind == "physical":
            input_name = f"Physical input {index + 1}"
            input_source = f"PHY{index + 1}"
        else:
            can_instance = int(input_data.get("canInstance", 0)) + 1
            can_id = int(input_data.get("canId", 0))
            input_name = f"CAN input {index + 1}"
            input_source = f"CAN{can_instance} 0x{can_id:03X}"

        input_type = "Digital" if int(input_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else "Analog"

        self._set_item(row_index, 0, input_name)
        self._set_item(row_index, 1, input_source)
        self._set_item(row_index, 2, input_type)
        self._set_item(row_index, 3, self._format_outputs(row["outputs"]))
        self.table.setCellWidget(row_index, 4, row["control"])
        self.table.setCellWidget(row_index, 5, row["voltage_chart"])
        self.table.setCellWidget(row_index, 6, row["current_chart"])

    def _set_item(self, row, column, text):
        item = QTableWidgetItem(str(text))
        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
        item.setToolTip(str(text))
        self.table.setItem(row, column, item)

    def _format_outputs(self, outputs):
        labels = []
        for entry in outputs or []:
            if isinstance(entry, dict):
                label = entry.get("name")
                if not label:
                    index = entry.get("index")
                    label = f"OUT_{int(index) + 1}" if index is not None else "OUT"
                labels.append(str(label))
            else:
                labels.append(str(entry))
        return ", ".join(labels) if labels else "-"

    def _handle_value_changed(self, input_data, value):
        try:
            arbitration_id = int(input_data.get("canId", 0))
            data_type = int(input_data.get("dataType", 0))
            offset = int(input_data.get("offset", 0))
            payload = bytearray(8)
            encoded_value = _encode_can_slider_value(data_type, value)
            if offset < 0 or offset + len(encoded_value) > len(payload):
                raise ValueError("Value does not fit into an 8-byte CAN frame at the selected offset")
            payload[offset : offset + len(encoded_value)] = encoded_value
        except Exception as exc:
            QMessageBox.critical(self, "Control send failed", str(exc))
            return

        self.can_frame_requested.emit(arbitration_id, bytes(payload))
