import math

from PyQt5.QtCore import QPointF, Qt
from PyQt5.QtGui import QColor, QPainter, QPen, QPolygonF
from PyQt5.QtWidgets import QSizePolicy, QWidget


class I2TChartWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._nominal_current_ma = 1000
        self._i2t_threshold = 10000000
        self._enabled = True
        self.setMinimumWidth(320)
        self.setMaximumWidth(460)
        self.setMinimumHeight(300)
        self.setContentsMargins(10, 10, 10, 10)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    def set_parameters(self, nominal_current_ma, i2t_threshold, enabled=True):
        self._nominal_current_ma = max(1, int(nominal_current_ma))
        self._i2t_threshold = max(1, int(i2t_threshold))
        self._enabled = bool(enabled)
        self.update()

    @staticmethod
    def _linear_map(value, minimum, maximum, start, end):
        minimum = max(1e-9, float(minimum))
        maximum = max(minimum * 1.0001, float(maximum))
        value = max(minimum, min(maximum, float(value)))
        if abs(maximum - minimum) < 1e-9:
            return float(start)
        ratio = (value - minimum) / (maximum - minimum)
        return float(start) + (ratio * (float(end) - float(start)))

    @staticmethod
    def _log_map(value, minimum, maximum, start, end):
        minimum = max(1e-9, float(minimum))
        maximum = max(minimum * 1.0001, float(maximum))
        value = max(minimum, min(maximum, float(value)))
        if abs(maximum - minimum) < 1e-9:
            return float(start)
        log_min = math.log10(minimum)
        log_max = math.log10(maximum)
        if abs(log_max - log_min) < 1e-9:
            return float(start)
        ratio = (math.log10(value) - log_min) / (log_max - log_min)
        return float(start) + (ratio * (float(end) - float(start)))

    @staticmethod
    def _format_time_label(value_s):
        rounded = math.ceil(max(0.0, float(value_s)) * 100.0 - 1e-9) / 100.0
        return f"{rounded:.2f}s"

    @staticmethod
    def _format_current_label(value_a):
        if value_a < 10.0:
            return f"{value_a:.1f}A"
        if abs(value_a - round(value_a)) > 0.05:
            return f"{value_a:.1f}A"
        return f"{value_a:.0f}A"

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        rect = self.rect().adjusted(0, 0, -1, -1)
        painter.fillRect(rect, QColor("#111111"))

        left_margin = 58
        right_margin = 10
        top_margin = 40
        bottom_margin = 42
        plot_rect = rect.adjusted(left_margin, top_margin, -right_margin, -bottom_margin)
        if plot_rect.width() <= 0 or plot_rect.height() <= 0:
            return

        nominal_current_a = max(0.01, min(100.0, self._nominal_current_ma / 1000.0))
        i2t_threshold_a2s = max(1e-9, float(self._i2t_threshold)) / 1000000000.0
        low_overcurrent_delta_a = 0.01

        y_over_min = 0.01
        y_over_max = 100.0

        x_min = 0.0
        log_x_min = max(0.01, x_min)
        log_x_max = 10000.0

        x_ticks = [0.01, 0.1, 1.0, 10.0, 100.0, 1000.0, 10000.0]
        x_ticks = [tick for tick in x_ticks if log_x_min <= tick <= log_x_max]

        painter.setPen(QPen(QColor("#2A2A2A"), 1))
        for tick in x_ticks:
            x = self._log_map(tick, log_x_min, log_x_max, plot_rect.left(), plot_rect.right())
            painter.drawLine(int(x), plot_rect.top(), int(x), plot_rect.bottom())

        y_ticks = [0.01, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0]
        y_ticks = [tick for tick in y_ticks if y_over_min <= tick <= y_over_max]
        y_ticks = sorted({round(tick, 6) for tick in y_ticks})
        for tick in y_ticks:
            y = self._log_map(tick, y_over_min, y_over_max, plot_rect.bottom(), plot_rect.top())
            painter.drawLine(plot_rect.left(), int(y), plot_rect.right(), int(y))

        painter.setPen(QPen(QColor("#8A8A8A"), 1))
        painter.drawRect(plot_rect)
        painter.setPen(QColor("#B8B8B8"))
        painter.drawText(rect.adjusted(10, 2, -10, -2), Qt.AlignTop | Qt.AlignLeft, "Overcurrent (A)")
        painter.drawText(rect.adjusted(10, 2, -10, -2), Qt.AlignBottom | Qt.AlignRight, "Time (s)")

        for tick in y_ticks:
            y = self._log_map(tick, y_over_min, y_over_max, plot_rect.bottom(), plot_rect.top())
            painter.drawText(0, int(y) - 7, 54, 14, Qt.AlignRight | Qt.AlignVCenter, self._format_current_label(tick))

        for tick in x_ticks:
            x = self._log_map(tick, log_x_min, log_x_max, plot_rect.left(), plot_rect.right())
            painter.drawText(int(x) - 28, plot_rect.bottom() + 14, 56, 16, Qt.AlignHCenter, self._format_time_label(tick))

        curve_points = []
        sample_count = 240

        sample_overcurrent_min = max(0.01, y_over_min, low_overcurrent_delta_a)
        sample_overcurrent_max = max(y_over_max, sample_overcurrent_min * 1.001)

        for index in range(sample_count):
            ratio = index / max(1, sample_count - 1)
            overcurrent_a = sample_overcurrent_min * ((sample_overcurrent_max / sample_overcurrent_min) ** ratio)

            if overcurrent_a < 0.01:
                continue

            denominator = (nominal_current_a + overcurrent_a) ** 2 - (nominal_current_a ** 2)
            if denominator <= 0:
                continue

            time_s = i2t_threshold_a2s / denominator

            if time_s < log_x_min or time_s > log_x_max:
                continue

            x = self._log_map(time_s, log_x_min, log_x_max, plot_rect.left(), plot_rect.right())
            y = self._log_map(overcurrent_a, y_over_min, y_over_max, plot_rect.bottom(), plot_rect.top())

            curve_points.append(QPointF(float(x), float(y)))

        curve_points.sort(key=lambda pt: pt.x())

        if len(curve_points) >= 2:
            painter.setPen(QPen(QColor("#09BC8A" if self._enabled else "#6C8B82"), 2))
            painter.drawPolyline(QPolygonF(curve_points))

        painter.end()
