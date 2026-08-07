from collections import deque
import time

import pyqtgraph as pg
from PyQt5.QtCore import QEvent, QPoint, Qt, pyqtSignal
from PyQt5.QtGui import QColor, QLinearGradient, QPainter, QPen
from PyQt5.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QFrame,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMenu,
    QMessageBox,
    QSizePolicy,
    QScrollArea,
    QSplitter,
    QVBoxLayout,
    QWidget,
)

from pdm_shared import OUT_STATUS_MAP

LINE_HISTORY_SECONDS = 10.0
MAX_LINE_CHARTS = 6
MAX_BAR_CHARTS = 16
BASE_BAR_CHART_HEIGHT = 44
MAX_LINE_CHART_HEIGHT = 190
MAX_BAR_CHART_HEIGHT = 84

OUT_STATE_LOG_MAP = {0: "OFF", 1: "ON", 2: "ERROR"}
STATUS_CODE_ORDER = sorted(OUT_STATUS_MAP.keys())
STATUS_CODE_TO_INDEX = {code: index for index, code in enumerate(STATUS_CODE_ORDER)}


class VerticalHintLabel(QLabel):
    def __init__(self, text, parent=None):
        super().__init__(text, parent)
        self.setAlignment(Qt.AlignCenter)
        self.setStyleSheet("color: #7A7A7A; font-size: 11px;")
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.TextAntialiasing, True)
        painter.translate(self.width() / 2.0, self.height() / 2.0)
        painter.rotate(-90)
        painter.translate(-self.height() / 2.0, -self.width() / 2.0)
        text_rect = self.rect().adjusted(0, 0, 0, 0)
        painter.setPen(QColor("#7A7A7A"))
        painter.drawText(text_rect, Qt.AlignCenter | Qt.TextWordWrap, self.text())
        painter.end()


class SignalPickerDialog(QDialog):
    def __init__(self, signals, parent=None, title="Select signal"):
        super().__init__(parent)
        self.setWindowTitle(title)
        self.setModal(True)
        self.resize(560, 520)

        self._signals = list(signals)
        self.selected_signal = None

        layout = QVBoxLayout(self)

        self.filter_edit = QLineEdit()
        self.filter_edit.setPlaceholderText("Filter signals (example: batt, out4.state, soc)")
        layout.addWidget(self.filter_edit)

        self.signal_list = QListWidget()
        layout.addWidget(self.signal_list, stretch=1)

        button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        layout.addWidget(button_box)

        button_box.accepted.connect(self._accept)
        button_box.rejected.connect(self.reject)
        self.filter_edit.textChanged.connect(self._refresh)
        self.signal_list.itemDoubleClicked.connect(lambda _item: self._accept())

        self._refresh()

    def _refresh(self):
        filter_text = self.filter_edit.text().strip().lower()
        self.signal_list.clear()

        if filter_text:
            filtered = [
                path
                for path in self._signals
                if filter_text in path.lower() or path.lower().replace(".", "").startswith(filter_text)
            ]
        else:
            filtered = list(self._signals)

        for path in filtered:
            item = QListWidgetItem(path)
            self.signal_list.addItem(item)

        if self.signal_list.count() > 0:
            self.signal_list.setCurrentRow(0)

    def _accept(self):
        item = self.signal_list.currentItem()
        if item is None:
            QMessageBox.information(self, "No signal selected", "Select a signal first.")
            return
        self.selected_signal = item.text()
        self.accept()


class LineChartWidget(QFrame):
    remove_requested = pyqtSignal(object)
    add_chart_requested = pyqtSignal()

    def __init__(self, signal_provider, menu_open_notifier=None, parent=None):
        super().__init__(parent)
        self._signal_provider = signal_provider
        self._menu_open_notifier = menu_open_notifier
        self._signals = []
        self._histories = {}
        self._curves = {}
        self._curve_scale = {}
        self._color_index = 0

        self._plot_item = None
        self._left_view = None
        self._right_view = None
        self._legend_left = None
        self._legend_right = None

        self.setFrameShape(QFrame.StyledPanel)
        self.setFrameShadow(QFrame.Raised)

        layout = QVBoxLayout(self)
        # Keep top spacing inside the chart frame so padding stays within the gray box.
        layout.setContentsMargins(4, 0, 4, 4)

        self.plot = pg.PlotWidget()
        self.plot.setBackground("#1E1E1E")
        self.plot.setMenuEnabled(False)
        self.plot.setContextMenuPolicy(Qt.NoContextMenu)
        # Inner top offset stays inside the plot/gray region.
        self.plot.getPlotItem().layout.setContentsMargins(0, 8, 0, 0)
        layout.addWidget(self.plot)
        self._setup_plot()
        self.plot.scene().sigMouseClicked.connect(self._on_plot_scene_clicked)

    def _setup_plot(self):
        self._plot_item = self.plot.getPlotItem()
        self._left_view = self._plot_item.getViewBox()
        self._left_view.setMouseEnabled(x=False, y=False)
        self._left_view.setMenuEnabled(False)
        self._plot_item.setMouseEnabled(x=False, y=False)
        self._plot_item.setMenuEnabled(False)
        self.plot.setMenuEnabled(False)
        self._plot_item.showGrid(x=True, y=True, alpha=0.15)
        self._plot_item.setLabel("bottom", "Time", units="s")
        self._plot_item.setLabel("left", "Scale A")
        self._plot_item.showAxis("right")
        self._plot_item.getAxis("right").setLabel("Scale B")
        self._plot_item.getAxis("right").setStyle(showValues=False)

        self._right_view = pg.ViewBox()
        self._right_view.setMouseEnabled(x=False, y=False)
        self._right_view.setMenuEnabled(False)
        self._plot_item.scene().addItem(self._right_view)
        self._plot_item.getAxis("right").linkToView(self._right_view)
        self._right_view.setXLink(self._left_view)
        self._left_view.sigResized.connect(self._sync_right_view_geometry)
        self._sync_right_view_geometry()

        self._create_legends()

    def _sync_right_view_geometry(self):
        if self._right_view is None or self._left_view is None:
            return
        self._right_view.setGeometry(self._left_view.sceneBoundingRect())
        self._right_view.linkedViewChanged(self._left_view, self._right_view.XAxis)

    def _create_legends(self):
        for legend in (self._legend_left, self._legend_right):
            if legend is not None:
                try:
                    legend.scene().removeItem(legend)
                except Exception:
                    pass

        self._legend_left = pg.LegendItem(colCount=1)
        self._legend_left.setParentItem(self._plot_item.vb)
        self._legend_left.anchor((0, 0), (0, 0), offset=(8, 8))

        self._legend_right = pg.LegendItem(colCount=1)
        self._legend_right.setParentItem(self._plot_item.vb)
        self._legend_right.anchor((1, 0), (1, 0), offset=(-8, 8))

    def to_dict(self):
        return {"signals": list(self._signals)}

    def _on_plot_scene_clicked(self, mouse_event):
        if mouse_event.button() != Qt.RightButton:
            return
        mouse_event.accept()
        scene_pos = mouse_event.scenePos()
        view_pos = self.plot.mapFromScene(scene_pos)
        self._show_context_menu(QPoint(int(view_pos.x()), int(view_pos.y())))

    def set_signals(self, signals):
        for curve in list(self._curves.values()):
            try:
                self._plot_item.removeItem(curve)
            except Exception:
                pass
            try:
                self._right_view.removeItem(curve)
            except Exception:
                pass

        self._signals = []
        self._histories = {}
        self._curves = {}
        self._curve_scale = {}
        self._color_index = 0
        self._plot_item.clearPlots()
        self._refresh_legend()
        self._plot_item.setLabel("left", "Scale A")
        self._plot_item.getAxis("right").setLabel("Scale B")
        self._plot_item.getAxis("right").setStyle(showValues=False)

        for signal_path in signals:
            self.add_signal(signal_path)

    def add_signal(self, signal_path):
        if not signal_path or signal_path in self._signals:
            return

        scale_meta = _infer_signal_scale(signal_path)
        existing_scales = []
        for current_path in self._signals:
            key = self._curve_scale.get(current_path)
            if key and key not in existing_scales:
                existing_scales.append(key)

        if scale_meta["key"] not in existing_scales and len(existing_scales) >= 2:
            QMessageBox.warning(
                self,
                "Signal scale limit",
                "Only two different signal scales are allowed per line chart.",
            )
            return

        color = pg.intColor(self._color_index)
        self._color_index += 1
        curve = pg.PlotDataItem([], [], pen=pg.mkPen(color=color, width=1.8))

        if not existing_scales or scale_meta["key"] == existing_scales[0]:
            self._plot_item.addItem(curve)
        else:
            self._right_view.addItem(curve)

        self._signals.append(signal_path)
        self._histories[signal_path] = deque(maxlen=1200)
        self._curves[signal_path] = curve
        self._curve_scale[signal_path] = scale_meta["key"]
        self._refresh_legend()
        self._refresh_axes_labels()

    def remove_signal(self, signal_path):
        if signal_path not in self._signals:
            return

        self._signals.remove(signal_path)
        curve = self._curves.pop(signal_path, None)
        self._curve_scale.pop(signal_path, None)
        if curve is not None:
            try:
                self._plot_item.removeItem(curve)
            except Exception:
                pass
            try:
                self._right_view.removeItem(curve)
            except Exception:
                pass
        self._histories.pop(signal_path, None)
        self._refresh_legend()
        self._refresh_axes_labels()

    def update_snapshot(self, timestamp_s, snapshot):
        min_x = max(0.0, float(timestamp_s) - LINE_HISTORY_SECONDS)
        max_x = min_x + LINE_HISTORY_SECONDS

        if not self._signals:
            self._left_view.setXRange(min_x, max_x, padding=0)
            self._right_view.setXRange(min_x, max_x, padding=0)
            return

        values_by_scale = {}

        for signal_path in list(self._signals):
            value = _resolve_path(snapshot, signal_path)
            scale_meta = _infer_signal_scale(signal_path)
            converted_value = _convert_value_for_scale(value, scale_meta)
            if isinstance(converted_value, bool):
                converted_value = int(converted_value)
            if isinstance(converted_value, (int, float)):
                self._histories[signal_path].append((float(timestamp_s), float(converted_value)))

            history = self._histories.get(signal_path) or deque()
            while history and history[0][0] < min_x:
                history.popleft()

            if len(history) >= 2:
                x_data = [point[0] for point in history]
                y_data = [point[1] for point in history]
                self._curves[signal_path].setData(x_data, y_data)
                scale_key = self._curve_scale.get(signal_path)
                values_by_scale.setdefault(scale_key, []).extend(y_data)
            else:
                self._curves[signal_path].setData([], [])
                scale_key = self._curve_scale.get(signal_path)
                values_by_scale.setdefault(scale_key, []).extend([point[1] for point in history])

        self._left_view.setXRange(min_x, max_x, padding=0)
        self._right_view.setXRange(min_x, max_x, padding=0)
        self._apply_axis_ranges(values_by_scale)

    def _show_context_menu(self, pos):
        if callable(self._menu_open_notifier):
            self._menu_open_notifier()
        menu = QMenu(self)

        add_signal_action = menu.addAction("Add signal")
        remove_signal_action = menu.addAction("Remove signal")
        menu.addSeparator()
        add_chart_action = menu.addAction("Add chart")
        remove_chart_action = menu.addAction("Remove chart")
        if not self._signals:
            remove_signal_action.setEnabled(False)

        action = menu.exec_(self.plot.mapToGlobal(pos))
        if action == add_signal_action:
            available_signals = self._signal_provider() or []
            if not available_signals:
                QMessageBox.information(self, "No signals", "No signals available yet.")
                return
            picker = SignalPickerDialog(available_signals, self, title="Add signal to line chart")
            if picker.exec_() == QDialog.Accepted and picker.selected_signal:
                self.add_signal(picker.selected_signal)
            return

        if action == remove_signal_action:
            if not self._signals:
                return
            sub_menu = QMenu("Select signal", self)
            actions = []
            for signal_path in self._signals:
                actions.append((sub_menu.addAction(signal_path), signal_path))
            selected = sub_menu.exec_(self.plot.mapToGlobal(pos))
            if selected is None:
                return
            for item_action, signal_path in actions:
                if selected == item_action:
                    self.remove_signal(signal_path)
                    break

        if action == add_chart_action:
            self.add_chart_requested.emit()
            return

        if action == remove_chart_action:
            self.remove_requested.emit(self)
            return

    def _refresh_legend(self):
        if self._legend_left is None or self._legend_right is None:
            self._create_legends()

        self._legend_left.clear()
        self._legend_right.clear()

        scale_order = []
        for signal_path in self._signals:
            scale_key = self._curve_scale.get(signal_path)
            if scale_key and scale_key not in scale_order:
                scale_order.append(scale_key)

        left_key = scale_order[0] if scale_order else None
        right_key = scale_order[1] if len(scale_order) >= 2 else None

        for signal_path in self._signals:
            curve = self._curves.get(signal_path)
            if curve is not None:
                scale_key = self._curve_scale.get(signal_path)
                if scale_key == left_key:
                    self._legend_left.addItem(curve, signal_path)
                elif scale_key == right_key:
                    self._legend_right.addItem(curve, signal_path)
                else:
                    self._legend_left.addItem(curve, signal_path)

        self._legend_left.setVisible(len(self._signals) > 0)
        self._legend_right.setVisible(right_key is not None)

    def _refresh_axes_labels(self):
        scale_order = []
        for signal_path in self._signals:
            scale_key = self._curve_scale.get(signal_path)
            if scale_key and scale_key not in scale_order:
                scale_order.append(scale_key)

        left_axis = self._plot_item.getAxis("left")
        right_axis = self._plot_item.getAxis("right")
        left_axis.enableAutoSIPrefix(False)
        right_axis.enableAutoSIPrefix(False)

        if not scale_order:
            self._plot_item.setLabel("left", "Scale A")
            right_axis.setLabel("Scale B")
            right_axis.setStyle(showValues=False)
            left_axis.setTicks(None)
            right_axis.setTicks(None)
            return

        left_meta = _scale_meta_by_key(scale_order[0])
        self._plot_item.setLabel("left", left_meta["axis"], units=left_meta["unit"])
        left_axis.setTicks([left_meta["ticks"]] if left_meta.get("ticks") else None)
        left_axis.setStyle(hideOverlappingLabels=not bool(left_meta.get("ticks")))
        left_axis.setWidth(92 if left_meta.get("ticks") else 60)

        if len(scale_order) >= 2:
            right_meta = _scale_meta_by_key(scale_order[1])
            right_axis.setLabel(right_meta["axis"], units=right_meta["unit"])
            right_axis.setStyle(showValues=True)
            right_axis.setTicks([right_meta["ticks"]] if right_meta.get("ticks") else None)
            right_axis.setStyle(showValues=True, hideOverlappingLabels=not bool(right_meta.get("ticks")))
        else:
            right_axis.setLabel("Scale B")
            right_axis.setStyle(showValues=False)
            right_axis.setTicks(None)

    def _apply_axis_ranges(self, values_by_scale):
        scale_order = []
        for signal_path in self._signals:
            scale_key = self._curve_scale.get(signal_path)
            if scale_key and scale_key not in scale_order:
                scale_order.append(scale_key)

        if scale_order:
            left_scale = scale_order[0]
            left_values = values_by_scale.get(left_scale, [])
            left_min, left_max = _compute_scale_range(left_scale, left_values)
            left_padding = 0.10 if left_scale in {"status", "state"} else 0
            self._left_view.setYRange(left_min, left_max, padding=left_padding)

        if len(scale_order) >= 2:
            right_scale = scale_order[1]
            right_values = values_by_scale.get(right_scale, [])
            right_min, right_max = _compute_scale_range(right_scale, right_values)
            right_padding = 0.10 if right_scale in {"status", "state"} else 0
            self._right_view.setYRange(right_min, right_max, padding=right_padding)


class BarGaugeWidget(QFrame):
    remove_requested = pyqtSignal(object)
    add_chart_requested = pyqtSignal()

    def __init__(self, signal_provider, snapshot_provider=None, menu_open_notifier=None, parent=None):
        super().__init__(parent)
        self._signal_provider = signal_provider
        self._snapshot_provider = snapshot_provider
        self._menu_open_notifier = menu_open_notifier
        self.signal_path = None
        self.min_value = 0.0
        self.max_value = 50.0
        self.current_value = None

        self.setMinimumHeight(1)
        self.setFrameShape(QFrame.StyledPanel)
        self.setFrameShadow(QFrame.Plain)

        self.label = QLabel("No signal")
        self.label.setAlignment(Qt.AlignCenter)
        self.label.setAttribute(Qt.WA_TransparentForMouseEvents, True)
        self.label.setStyleSheet("background: transparent; color: #E0E0E0; font-size: 18px; font-weight: bold;")

        self.signal_label = QLabel("-")
        self.signal_label.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)
        self.signal_label.setStyleSheet("color: #A5A5A5; font-size: 10px;")
        self.signal_label.setAttribute(Qt.WA_TransparentForMouseEvents, True)

        self.unit_label = QLabel("Y: N/A")
        self.unit_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.unit_label.setStyleSheet("color: #A5A5A5; font-size: 10px;")
        self.unit_label.setAttribute(Qt.WA_TransparentForMouseEvents, True)

        header_row = QHBoxLayout()
        header_row.setContentsMargins(0, 0, 0, 0)
        header_row.setSpacing(6)
        header_row.addWidget(self.signal_label, stretch=1)
        header_row.addWidget(self.unit_label, stretch=0)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 6, 6, 6)
        layout.setSpacing(2)
        layout.addLayout(header_row)
        layout.addStretch(1)
        layout.addWidget(self.label)
        layout.addStretch(1)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    def to_dict(self):
        return {
            "signal": self.signal_path,
            "min": self.min_value,
            "max": self.max_value,
        }

    def apply_dict(self, data):
        if not isinstance(data, dict):
            return
        self.signal_path = data.get("signal")
        try:
            self.min_value = float(data.get("min", 0.0))
        except Exception:
            self.min_value = 0.0
        try:
            self.max_value = float(data.get("max", 100.0))
        except Exception:
            self.max_value = 100.0

    def update_snapshot(self, snapshot):
        if not self.signal_path:
            self.current_value = None
            self.signal_label.setText("-")
            self.unit_label.setText("Y: N/A")
            self.label.setText("No signal")
            self.update()
            return

        value = _resolve_path(snapshot, self.signal_path)
        scale_meta = _infer_signal_scale(self.signal_path)
        converted_value = _convert_value_for_scale(value, scale_meta)
        self.current_value = converted_value
        self.signal_label.setText(self.signal_path)
        self.unit_label.setText(f"Y: {scale_meta['display_unit']}")
        self.label.setText(_format_bar_label(self.signal_path, converted_value))
        self.update()

    def paintEvent(self, event):
        super().paintEvent(event)
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, False)

        rect = self.rect().adjusted(3, 3, -3, -3)

        header_bottom = max(self.signal_label.geometry().bottom(), self.unit_label.geometry().bottom())
        body_top = max(rect.top() + 1, header_bottom + 4)
        bar_rect = rect.adjusted(1, body_top - rect.top(), -1, -1)
        if bar_rect.height() < 2:
            bar_rect = rect.adjusted(1, 1, -1, -1)

        painter.setPen(Qt.NoPen)
        painter.setBrush(QColor("#121212"))
        painter.drawRect(bar_rect)

        if isinstance(self.current_value, bool):
            numeric_value = float(int(self.current_value))
        elif isinstance(self.current_value, (int, float)):
            numeric_value = float(self.current_value)
        else:
            numeric_value = None

        ratio = 0.0
        if numeric_value is not None and self.max_value > self.min_value:
            ratio = (numeric_value - self.min_value) / (self.max_value - self.min_value)
            ratio = max(0.0, min(1.0, ratio))

        fill_rect = bar_rect.adjusted(1, 1, -1, -1)
        if fill_rect.width() > 0 and fill_rect.height() > 0:
            # Draw a continuous min->max gradient scale (blue to pink) across the full gauge width.
            gradient = QLinearGradient(fill_rect.left(), 0, fill_rect.right(), 0)
            gradient.setColorAt(0.00, QColor("#1F6FEB"))
            gradient.setColorAt(1.00, QColor("#FF4FA3"))
            painter.setBrush(gradient)
            painter.drawRect(fill_rect)

            # Dim the unfilled region so the active value is readable while preserving the scale.
            cut_x = fill_rect.left() + int(fill_rect.width() * ratio)
            if cut_x < fill_rect.right():
                painter.setBrush(QColor(18, 18, 18, 215))
                painter.drawRect(cut_x, fill_rect.top(), fill_rect.right() - cut_x + 1, fill_rect.height())

            # Value marker line at the current ratio.
            marker_x = min(fill_rect.right(), max(fill_rect.left(), cut_x))
            painter.setPen(QPen(QColor("#EAEAEA"), 1))
            painter.drawLine(marker_x, fill_rect.top(), marker_x, fill_rect.bottom())

        painter.setBrush(Qt.NoBrush)
        painter.setPen(QPen(QColor("#333333"), 1))
        painter.drawRect(rect)

    def _show_context_menu(self, pos):
        if callable(self._menu_open_notifier):
            self._menu_open_notifier()
        menu = QMenu(self)
        add_signal_action = menu.addAction("Change signal" if self.signal_path else "Add signal")
        remove_signal_action = menu.addAction("Remove signal")
        menu.addSeparator()
        add_chart_action = menu.addAction("Add chart")
        remove_action = menu.addAction("Remove chart")
        menu.addSeparator()
        set_min_action = menu.addAction("Set min value")
        set_max_action = menu.addAction("Set max value")

        if not self.signal_path:
            remove_signal_action.setEnabled(False)

        action = menu.exec_(self.mapToGlobal(pos))
        if action == add_signal_action:
            available_signals = self._signal_provider() or []
            if not available_signals:
                QMessageBox.information(self, "No signals", "No signals available yet.")
                return
            picker = SignalPickerDialog(available_signals, self, title="Add signal to bar chart")
            if picker.exec_() == QDialog.Accepted and picker.selected_signal:
                self.signal_path = picker.selected_signal
                self.signal_label.setText(self.signal_path)
                self._apply_default_gradient_range_for_signal()
            return

        if action == remove_signal_action:
            self.signal_path = None
            self.current_value = None
            self.signal_label.setText("-")
            self.unit_label.setText("Y: N/A")
            self.label.setText("No signal")
            self.update()
            return

        if action == add_chart_action:
            self.add_chart_requested.emit()
            return

        if action == remove_action:
            self.remove_requested.emit(self)
            return

        if action == set_min_action:
            value = _prompt_float(self, "Set min value", self.min_value)
            if value is not None:
                self.min_value = value
                self.update()
            return

        if action == set_max_action:
            value = _prompt_float(self, "Set max value", self.max_value)
            if value is not None:
                self.max_value = value
                self.update()
            return

    def _apply_default_gradient_range_for_signal(self):
        if not self.signal_path:
            return
        scale_meta = _infer_signal_scale(self.signal_path)
        gradient_min, gradient_max = scale_meta.get("bar_gradient_range", (0.0, 1.0))

        # For channel current bars, use configured nominal current as max when available.
        if scale_meta.get("key") == "current":
            channel_key = _channel_from_signal_path(self.signal_path)
            if channel_key.startswith("out"):
                snapshot = self._snapshot_provider() if callable(self._snapshot_provider) else {}
                nominal_raw = (
                    _resolve_path(snapshot, f"{channel_key}.cfg.soc.nominalCurrent")
                    or _resolve_path(snapshot, f"{channel_key}.cfg.oc.nominalThreshold")
                    or _resolve_path(snapshot, f"{channel_key}.cfg.oc.nominalTreshold")
                )
                if isinstance(nominal_raw, (int, float)) and float(nominal_raw) > 0:
                    nominal_a = _convert_value_for_scale(nominal_raw, scale_meta)
                    if isinstance(nominal_a, (int, float)) and float(nominal_a) > 0:
                        gradient_max = float(nominal_a)

        self.min_value = float(gradient_min)
        self.max_value = float(gradient_max)


class LogViewTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        self._latest_snapshot = {}
        self._latest_config_snapshot = {}
        self._last_timestamp = 0.0

        self._left_empty_hint = None
        self._right_empty_hint = None
        self._suppress_section_menu_until = 0.0
        self._paused = False

        root_layout = QVBoxLayout(self)
        root_layout.setContentsMargins(8, 8, 8, 8)
        root_layout.setSpacing(8)

        splitter = QSplitter(Qt.Horizontal)
        root_layout.addWidget(splitter, stretch=1)

        self.left_content = QWidget()
        self.left_layout = QVBoxLayout(self.left_content)
        self.left_layout.setContentsMargins(0, 0, 0, 0)
        self.left_layout.setSpacing(8)
        self.left_layout.setAlignment(Qt.AlignTop)
        self.left_layout.addStretch(1)

        self.left_scroll = QScrollArea()
        self.left_scroll.setWidgetResizable(True)
        self.left_scroll.setWidget(self.left_content)
        self.left_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.left_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.left_scroll.setContextMenuPolicy(Qt.CustomContextMenu)
        self.left_scroll.customContextMenuRequested.connect(self._show_left_section_menu)
        self.left_scroll.viewport().installEventFilter(self)
        splitter.addWidget(self.left_scroll)

        self.right_content = QWidget()
        self.right_layout = QVBoxLayout(self.right_content)
        self.right_layout.setContentsMargins(0, 0, 0, 0)
        self.right_layout.setSpacing(6)
        self.right_layout.setAlignment(Qt.AlignTop)
        self.right_layout.addStretch(1)

        self.right_scroll = QScrollArea()
        self.right_scroll.setWidgetResizable(True)
        self.right_scroll.setWidget(self.right_content)
        self.right_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.right_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.right_scroll.setContextMenuPolicy(Qt.CustomContextMenu)
        self.right_scroll.customContextMenuRequested.connect(self._show_right_section_menu)
        self.right_scroll.viewport().installEventFilter(self)
        splitter.addWidget(self.right_scroll)

        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([1400, 450])

        self._line_charts = []
        self._bar_charts = []

        self._left_empty_hint = VerticalHintLabel("Right-click to add charts and signals to be logged.")
        self._right_empty_hint = VerticalHintLabel("Right-click to add charts and signals to be logged.")
        self.left_layout.insertWidget(self.left_layout.count() - 1, self._left_empty_hint)
        self.right_layout.insertWidget(self.right_layout.count() - 1, self._right_empty_hint)

        self.add_line_chart()
        self._sync_empty_hints()
        self._apply_chart_heights()

    def eventFilter(self, watched, event):
        if event.type() == QEvent.Resize:
            if watched is self.left_scroll.viewport() or watched is self.right_scroll.viewport():
                self._sync_empty_hints()
                self._apply_chart_heights()
        return super().eventFilter(watched, event)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._sync_empty_hints()
        self._apply_chart_heights()

    def _show_left_section_menu(self, pos):
        if time.time() < self._suppress_section_menu_until:
            return
        for chart in self._line_charts:
            if _point_in_viewport_widget(self.left_scroll.viewport(), chart, pos):
                return
        menu = QMenu(self)
        add_line_action = menu.addAction("Add line chart")
        action = menu.exec_(self.left_scroll.viewport().mapToGlobal(pos))
        if action == add_line_action:
            self.add_line_chart()

    def _show_right_section_menu(self, pos):
        if time.time() < self._suppress_section_menu_until:
            return
        for bar in self._bar_charts:
            if _point_in_viewport_widget(self.right_scroll.viewport(), bar, pos):
                return
        menu = QMenu(self)
        add_bar_action = menu.addAction("Add bar chart")
        action = menu.exec_(self.right_scroll.viewport().mapToGlobal(pos))
        if action == add_bar_action:
            self.add_bar_chart()

    def _available_signals(self):
        if not self._latest_snapshot:
            self._latest_snapshot = _build_signal_snapshot({}, self._latest_config_snapshot)
        flat = _flatten_snapshot(self._latest_snapshot)
        numeric_paths = [
            path
            for path, value in flat.items()
            if _is_numeric_signal(path, value)
        ]
        return sorted(numeric_paths)

    def _snapshot_for_widgets(self):
        return self._latest_snapshot if isinstance(self._latest_snapshot, dict) else {}

    def set_config_snapshot(self, config_snapshot):
        self._latest_config_snapshot = config_snapshot if isinstance(config_snapshot, dict) else {}
        if self._last_timestamp <= 0.0:
            self._latest_snapshot = _build_signal_snapshot({}, self._latest_config_snapshot)

    def add_line_chart(self):
        if len(self._line_charts) >= MAX_LINE_CHARTS:
            QMessageBox.information(self, "Limit reached", f"Maximum {MAX_LINE_CHARTS} line charts allowed.")
            return

        chart = LineChartWidget(self._available_signals, self._notify_chart_menu_opened, self)
        chart.remove_requested.connect(self._remove_line_chart)
        chart.add_chart_requested.connect(self.add_line_chart)
        self._line_charts.append(chart)
        self.left_layout.insertWidget(self.left_layout.count() - 1, chart)
        self._sync_empty_hints()
        self._apply_chart_heights()
        if self._last_timestamp > 0:
            chart.update_snapshot(self._last_timestamp, self._latest_snapshot)

    def add_bar_chart(self):
        if len(self._bar_charts) >= MAX_BAR_CHARTS:
            QMessageBox.information(self, "Limit reached", f"Maximum {MAX_BAR_CHARTS} bar charts allowed.")
            return

        bar = BarGaugeWidget(self._available_signals, self._snapshot_for_widgets, self._notify_chart_menu_opened, self)
        bar.remove_requested.connect(self._remove_bar_chart)
        bar.add_chart_requested.connect(self.add_bar_chart)
        self._bar_charts.append(bar)
        self.right_layout.insertWidget(self.right_layout.count() - 1, bar)
        self._sync_empty_hints()
        self._apply_chart_heights()

    def _remove_line_chart(self, chart):
        if chart in self._line_charts:
            self._line_charts.remove(chart)
            chart.setParent(None)
            chart.deleteLater()
            self._sync_empty_hints()
            self._apply_chart_heights()

    def _remove_bar_chart(self, bar):
        if bar in self._bar_charts:
            self._bar_charts.remove(bar)
            bar.setParent(None)
            bar.deleteLater()
            self._sync_empty_hints()
            self._apply_chart_heights()

    def update_from_packet(self, packet, config_snapshot):
        timestamp_s = float(packet.get("time", 0.0))
        self._latest_config_snapshot = config_snapshot if isinstance(config_snapshot, dict) else {}
        snapshot = _build_signal_snapshot(packet, config_snapshot)
        self._latest_snapshot = snapshot
        self._last_timestamp = timestamp_s

        if self._paused:
            return

        for chart in self._line_charts:
            chart.update_snapshot(timestamp_s, snapshot)

        for bar in self._bar_charts:
            bar.update_snapshot(snapshot)

    def is_paused(self):
        return bool(self._paused)

    def set_paused(self, paused):
        self._paused = bool(paused)
        if not self._paused and self._last_timestamp > 0:
            for chart in self._line_charts:
                chart.update_snapshot(self._last_timestamp, self._latest_snapshot)
            for bar in self._bar_charts:
                bar.update_snapshot(self._latest_snapshot)

    def toggle_paused(self):
        self.set_paused(not self._paused)
        return self._paused

    def to_dict(self):
        return {
            "lineCharts": [chart.to_dict() for chart in self._line_charts],
            "bars": [bar.to_dict() for bar in self._bar_charts],
            "historySeconds": LINE_HISTORY_SECONDS,
        }

    def apply_dict(self, data):
        if not isinstance(data, dict):
            return

        while self._line_charts:
            chart = self._line_charts.pop()
            chart.setParent(None)
            chart.deleteLater()

        while self._bar_charts:
            bar = self._bar_charts.pop()
            bar.setParent(None)
            bar.deleteLater()

        line_charts = data.get("lineCharts") if isinstance(data.get("lineCharts"), list) else []
        bars = data.get("bars") if isinstance(data.get("bars"), list) else []

        if not line_charts:
            self.add_line_chart()
        else:
            for item in line_charts[:MAX_LINE_CHARTS]:
                self.add_line_chart()
                if self._line_charts:
                    signals = item.get("signals") if isinstance(item, dict) else []
                    self._line_charts[-1].set_signals(signals if isinstance(signals, list) else [])

        for item in bars[:MAX_BAR_CHARTS]:
            self.add_bar_chart()
            if self._bar_charts:
                self._bar_charts[-1].apply_dict(item)

        self._sync_empty_hints()
        self._apply_chart_heights()

    def _sync_empty_hints(self):
        if self._left_empty_hint is not None:
            is_left_empty = len(self._line_charts) == 0
            self._left_empty_hint.setVisible(is_left_empty)
            self._left_empty_hint.setFixedHeight(0 if not is_left_empty else max(120, self.left_scroll.viewport().height() - 12))
        if self._right_empty_hint is not None:
            is_right_empty = len(self._bar_charts) == 0
            self._right_empty_hint.setVisible(is_right_empty)
            self._right_empty_hint.setFixedHeight(0 if not is_right_empty else max(120, self.right_scroll.viewport().height() - 12))

    def _notify_chart_menu_opened(self):
        self._suppress_section_menu_until = time.time() + 0.25

    def _apply_chart_heights(self):
        left_h = self.left_scroll.viewport().height() if self.left_scroll is not None else 0
        right_h = self.right_scroll.viewport().height() if self.right_scroll is not None else 0

        line_count = max(1, len(self._line_charts))
        bar_count = max(1, len(self._bar_charts))

        line_spacing = self.left_layout.spacing()
        bar_spacing = self.right_layout.spacing()
        line_margins = self.left_layout.contentsMargins()
        bar_margins = self.right_layout.contentsMargins()

        available_line_h = max(
            1,
            left_h - line_margins.top() - line_margins.bottom() - (line_count - 1) * line_spacing,
        )
        available_bar_h = max(
            1,
            right_h - bar_margins.top() - bar_margins.bottom() - (bar_count - 1) * bar_spacing,
        )

        line_height = max(1, int(available_line_h / line_count)) if left_h > 0 else 1
        bar_height = max(1, int(available_bar_h / bar_count)) if right_h > 0 else 1

        # Keep charts top-aligned and prevent oversized widgets when only a few are visible.
        line_height = min(MAX_LINE_CHART_HEIGHT, line_height)
        bar_height = min(MAX_BAR_CHART_HEIGHT, bar_height)

        for chart in self._line_charts:
            chart.setFixedHeight(line_height)

        for bar in self._bar_charts:
            bar.setFixedHeight(bar_height)


def _build_signal_snapshot(packet, config_snapshot):
    snapshot = {
        "sys": {},
        "imu": {},
        "phy": {},
    }

    sys_data = packet.get("sys") or {}
    snapshot["sys"] = {
        "status": sys_data.get("status", 0),
        "battVoltage": sys_data.get("batt", 0),
        "boardTemp": sys_data.get("core_temp", 0.0),
        "safetyLine": sys_data.get("safety", ""),
        "totalCurrent": sys_data.get("total_current", 0.0),
        "logicValidMask": sys_data.get("logicValidMask", 0),
        "systemLoad": sys_data.get("system_load", 0),
    }

    imu_data = packet.get("imu") or {}
    snapshot["imu"] = {
        "accX": imu_data.get("accX", 0.0),
        "accY": imu_data.get("accY", 0.0),
        "accZ": imu_data.get("accZ", 0.0),
        "pitch": imu_data.get("pitch", 0.0),
        "roll": imu_data.get("roll", 0.0),
        "yaw": imu_data.get("yaw", 0.0),
    }

    phy_data = packet.get("phy") or []
    phy_count = max(8, len(phy_data))
    snapshot["phy"] = {
        f"in{index + 1}": (phy_data[index] if index < len(phy_data) else 0)
        for index in range(phy_count)
    }

    channel_packets = packet.get("ch") or []
    config_channels = []
    if isinstance(config_snapshot, dict):
        config_channels = config_snapshot.get("channels") or []

    total_channels = max(16, len(channel_packets), len(config_channels))
    for index in range(total_channels):
        channel_data = channel_packets[index] if index < len(channel_packets) else {}
        key = f"out{index + 1}"

        config_entry = config_channels[index] if index < len(config_channels) and isinstance(config_channels[index], dict) else {}
        safety_cfg = config_entry.get("safety") if isinstance(config_entry.get("safety"), dict) else {}
        soc_cfg = safety_cfg.get("socCfg") if isinstance(safety_cfg.get("socCfg"), dict) else {}
        i2t_cfg = safety_cfg.get("i2tCfg") if isinstance(safety_cfg.get("i2tCfg"), dict) else {}

        snapshot[key] = {
            "name": channel_data.get("name", ""),
            "status": channel_data.get("status", 0),
            "state": channel_data.get("state", 0),
            "voltage": channel_data.get("voltage", 0),
            "current": channel_data.get("current", 0),
            "currentAvg": channel_data.get("current_avg", 0),
            "currentRms": channel_data.get("current_rms", 0),
            "pwmDuty": channel_data.get("pwm_duty", 0),
            "i2tHeat": channel_data.get("i2t_heat", 0),
            "socThreshold": channel_data.get("soc_threshold", 0),
            "cfg": {
                "soc": {
                    "enabled": soc_cfg.get("useSoc", False),
                    "nominalCurrent": soc_cfg.get("nominalThreshold", 0),
                    "allowInrush": soc_cfg.get("allowInrush", False),
                    "inrushInput": soc_cfg.get("inrushInput", 0),
                    "inrushWindowFromStart": soc_cfg.get("inrushWindowFromStart", 0),
                    "inrushThreshold": soc_cfg.get("inrushThreshold", 0),
                    "inrushTimeThreshold": soc_cfg.get("inrushTimeThreshold", 0),
                },
                "oc": {
                    "nominalTreshold": soc_cfg.get("nominalThreshold", 0),
                    "nominalThreshold": soc_cfg.get("nominalThreshold", 0),
                    "allowInrush": soc_cfg.get("allowInrush", False),
                    "inrushInput": soc_cfg.get("inrushInput", 0),
                    "inrushWindowFromStart": soc_cfg.get("inrushWindowFromStart", 0),
                    "inrushThreshold": soc_cfg.get("inrushThreshold", 0),
                    "inrushTimeThreshold": soc_cfg.get("inrushTimeThreshold", 0),
                },
                "i2t": {
                    "enabled": i2t_cfg.get("useI2t", False),
                    "nominalCurrent": i2t_cfg.get("nominalCurrent", 0),
                    "timeThreshold": i2t_cfg.get("timeThreshold", 0),
                    "i2tThreshold": i2t_cfg.get("i2tThreshold", 0),
                },
            },
        }

    return snapshot


def _flatten_snapshot(snapshot, prefix=""):
    flattened = {}
    if isinstance(snapshot, dict):
        for key, value in snapshot.items():
            path = f"{prefix}.{key}" if prefix else str(key)
            if isinstance(value, dict):
                flattened.update(_flatten_snapshot(value, path))
            else:
                flattened[path] = value
    return flattened


def _resolve_path(snapshot, path):
    current = snapshot
    for part in str(path).split("."):
        if not isinstance(current, dict):
            return None
        if part not in current:
            return None
        current = current.get(part)
    return current


def _prompt_float(parent, title, current_value):
    dialog = QDialog(parent)
    dialog.setWindowTitle(title)
    dialog.setModal(True)

    layout = QVBoxLayout(dialog)
    input_box = QLineEdit(str(current_value))
    layout.addWidget(input_box)

    button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
    layout.addWidget(button_box)

    result = {"value": None}

    def on_accept():
        try:
            result["value"] = float(input_box.text().strip())
            dialog.accept()
        except Exception:
            QMessageBox.warning(dialog, "Invalid number", "Enter a valid number.")

    button_box.accepted.connect(on_accept)
    button_box.rejected.connect(dialog.reject)

    if dialog.exec_() == QDialog.Accepted:
        return result["value"]
    return None


def _format_bar_label(signal_path, value):
    if value is None:
        return "-"
    if isinstance(value, bool):
        value_text = "true" if value else "false"
    elif isinstance(value, (int, float)):
        value_text = f"{value:.2f}" if isinstance(value, float) else str(value)
    else:
        value_text = str(value)
    return value_text


def _interpolate_color(start_hex, end_hex, ratio):
    ratio = max(0.0, min(1.0, float(ratio)))
    start = QColor(start_hex)
    end = QColor(end_hex)
    red = int(start.red() + (end.red() - start.red()) * ratio)
    green = int(start.green() + (end.green() - start.green()) * ratio)
    blue = int(start.blue() + (end.blue() - start.blue()) * ratio)
    return QColor(red, green, blue)


def _channel_from_signal_path(signal_path):
    if not signal_path:
        return "-"
    first = str(signal_path).split(".", 1)[0]
    return first


def _is_numeric_signal(signal_path, value):
    if isinstance(value, bool):
        return True
    if isinstance(value, (int, float)):
        return True
    scale_meta = _infer_signal_scale(signal_path)
    if scale_meta["key"] in {"status", "state"}:
        return isinstance(value, (int, float, bool))
    return False


def _find_parent_line_chart(widget):
    current = widget
    while current is not None:
        if isinstance(current, LineChartWidget):
            return current
        current = current.parentWidget()
    return None


def _find_parent_bar_chart(widget):
    current = widget
    while current is not None:
        if isinstance(current, BarGaugeWidget):
            return current
        current = current.parentWidget()
    return None


def _point_in_viewport_widget(viewport, widget, point):
    if viewport is None or widget is None:
        return False
    top_left = widget.mapTo(viewport, QPoint(0, 0))
    return widget.rect().translated(top_left).contains(point)


def _infer_signal_scale(signal_path):
    path = str(signal_path or "").lower()

    if ".state" in path:
        return {
            "key": "state",
            "axis": "State",
            "unit": "",
            "display_unit": "N/A",
            "min_span": 2.0,
            "force_zero": True,
            "converter": lambda raw: float(raw),
            "ticks": [(float(key), label) for key, label in sorted(OUT_STATE_LOG_MAP.items())],
            "bar_gradient_range": (0.0, 2.0),
        }

    if ".status" in path:
        return {
            "key": "status",
            "axis": "Status",
            "unit": "",
            "display_unit": "N/A",
            "min_span": max(1.0, float(len(STATUS_CODE_ORDER) - 1)),
            "force_zero": True,
            "converter": lambda raw: float(STATUS_CODE_TO_INDEX.get(int(raw), 0)),
            "ticks": [(float(index), OUT_STATUS_MAP[code]) for index, code in enumerate(STATUS_CODE_ORDER)],
            "bar_gradient_range": (0.0, max(1.0, float(len(STATUS_CODE_ORDER) - 1))),
        }

    if path.startswith("imu.acc"):
        return {
            "key": "accel_g",
            "axis": "Acceleration",
            "unit": "g",
            "display_unit": "[g]",
            "min_span": 4.0,
            "force_zero": False,
            "converter": lambda raw: float(raw),
            "ticks": None,
            "bar_gradient_range": (-16.0, 16.0),
        }

    if path.startswith("imu.pitch") or path.startswith("imu.roll") or path.startswith("imu.yaw"):
        return {
            "key": "dps",
            "axis": "Angular Rate",
            "unit": "dps",
            "display_unit": "[dps]",
            "min_span": 2000.0,
            "force_zero": False,
            "converter": lambda raw: float(raw),
            "ticks": None,
            "bar_gradient_range": (-1000.0, 1000.0),
        }

    if "voltage" in path or path.startswith("phy.in"):
        return {
            "key": "voltage",
            "axis": "Voltage",
            "unit": "V",
            "display_unit": "[V]",
            "min_span": 10.0,
            "force_zero": True,
            "converter": lambda raw: float(raw) / 1000.0,
            "ticks": None,
            "bar_gradient_range": (0.0, 18.0),
        }

    if any(token in path for token in ("current", "threshold", "nominalcurrent")):
        return {
            "key": "current",
            "axis": "Current",
            "unit": "A",
            "display_unit": "[A]",
            "min_span": 2.0,
            "force_zero": True,
            "converter": lambda raw: float(raw) / 1000.0,
            "ticks": None,
            "bar_gradient_range": (0.0, 50.0),
        }

    if "temp" in path:
        return {
            "key": "temperature",
            "axis": "Temperature",
            "unit": "C",
            "display_unit": "[*C]",
            "min_span": 40.0,
            "force_zero": False,
            "converter": lambda raw: float(raw),
            "ticks": None,
            "bar_gradient_range": (-40.0, 100.0),
        }

    if any(token in path for token in ("duty", "heat", "load")):
        return {
            "key": "percent",
            "axis": "Percent",
            "unit": "%",
            "display_unit": "%",
            "min_span": 100.0,
            "force_zero": True,
            "converter": lambda raw: float(raw),
            "ticks": None,
            "bar_gradient_range": (0.0, 100.0),
        }

    if any(token in path for token in ("mask", "enabled", "allow")):
        return {
            "key": "enum",
            "axis": "Enum",
            "unit": "",
            "display_unit": "N/A",
            "min_span": 1.0,
            "force_zero": True,
            "converter": lambda raw: float(raw),
            "ticks": None,
            "bar_gradient_range": (0.0, 1.0),
        }

    return {
        "key": "raw",
        "axis": "Raw",
        "unit": "",
        "display_unit": "N/A",
        "min_span": 1.0,
        "force_zero": False,
        "converter": lambda raw: float(raw),
        "ticks": None,
        "bar_gradient_range": (0.0, 1.0),
    }


def _scale_meta_by_key(scale_key):
    key_to_sample = {
        "state": "out1.state",
        "status": "out1.status",
        "accel_g": "imu.accX",
        "dps": "imu.roll",
        "voltage": "sys.battVoltage",
        "current": "out1.current",
        "temperature": "sys.boardTemp",
        "percent": "sys.systemLoad",
        "enum": "out1.cfg.soc.enabled",
        "raw": "out1.cfg.oc.nominalTreshold",
    }
    return _infer_signal_scale(key_to_sample.get(scale_key, scale_key))


def _convert_value_for_scale(value, scale_meta):
    if value is None:
        return None
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, (int, float)):
        try:
            return scale_meta["converter"](value)
        except Exception:
            return None
    return value


def _compute_scale_range(scale_key, values):
    meta = _scale_meta_by_key(scale_key)
    explicit_default = {
        "voltage": (0.0, 10.0),
        "current": (0.0, 2.0),
        "temperature": (10.0, 50.0),
        "accel_g": (-2.0, 2.0),
        "dps": (-1000.0, 1000.0),
        "state": (0.0, 2.0),
        "status": (0.0, max(1.0, float(len(STATUS_CODE_ORDER) - 1))),
    }
    if not values:
        if scale_key in explicit_default:
            return explicit_default[scale_key]
        lower = 0.0 if meta["force_zero"] else -meta["min_span"] / 2.0
        upper = lower + meta["min_span"]
        return lower, upper

    numeric_values = [float(value) for value in values if isinstance(value, (int, float))]
    if not numeric_values:
        if scale_key in explicit_default:
            return explicit_default[scale_key]
        lower = 0.0 if meta["force_zero"] else -meta["min_span"] / 2.0
        upper = lower + meta["min_span"]
        return lower, upper

    if meta["force_zero"]:
        lower = 0.0
        upper = max(numeric_values)
    else:
        lower = min(numeric_values)
        upper = max(numeric_values)

    if upper - lower < meta["min_span"]:
        upper = lower + meta["min_span"]

    if scale_key in explicit_default:
        default_low, default_high = explicit_default[scale_key]
        lower = min(lower, default_low)
        upper = max(upper, default_high)

    return lower, upper
