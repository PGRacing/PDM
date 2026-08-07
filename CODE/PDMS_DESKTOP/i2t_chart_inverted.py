import math

from PyQt5.QtWidgets import QWidget, QSizePolicy
from PyQt5.QtGui import QPainter, QPen, QColor, QPolygonF
from PyQt5.QtCore import Qt, QPointF


class I2TChartInvertedWidget(QWidget):
    """I2T protection curve with Amperage on X-axis, Time (0-100s) on Y-axis."""

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
        """Update curve parameters and refresh display."""
        self._nominal_current_ma = max(1, int(nominal_current_ma))
        self._i2t_threshold = max(1, int(i2t_threshold))
        self._enabled = bool(enabled)
        self.update()

    @staticmethod
    def _linear_map(value, minimum, maximum, start, end):
        """Map value linearly from [minimum, maximum] to [start, end]."""
        minimum = max(1e-9, float(minimum))
        maximum = max(minimum * 1.0001, float(maximum))
        value = max(minimum, min(maximum, float(value)))
        if abs(maximum - minimum) < 1e-9:
            return float(start)
        ratio = (value - minimum) / (maximum - minimum)
        return float(start) + (ratio * (float(end) - float(start)))

    @staticmethod
    def _log_map(value, minimum, maximum, start, end):
        """Map value logarithmically from [minimum, maximum] to [start, end]."""
        minimum = max(1e-9, float(minimum))
        maximum = max(minimum * 1.0001, float(maximum))
        value = max(minimum, min(maximum, float(value)))
        log_min = math.log10(minimum)
        log_max = math.log10(maximum)
        if abs(log_max - log_min) < 1e-9:
            return float(start)
        ratio = (math.log10(value) - log_min) / (log_max - log_min)
        return float(start) + (ratio * (float(end) - float(start)))

    @staticmethod
    def _format_time_label(value_s):
        """Format time label for Y-axis."""
        return f"{float(value_s):.0f}s"

    @staticmethod
    def _format_current_label(value_a):
        """Format current label for X-axis."""
        if value_a < 1.0:
            return f"{value_a:.2f}A"
        if value_a < 10.0:
            return f"{value_a:.1f}A"
        if abs(value_a - round(value_a)) > 0.05:
            return f"{value_a:.1f}A"
        return f"{value_a:.0f}A"

    def paintEvent(self, event):
        """Paint the I2T protection curve."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        rect = self.rect().adjusted(0, 0, -1, -1)
        painter.fillRect(rect, QColor("#111111"))

        left_margin = 50
        right_margin = 10
        top_margin = 40
        bottom_margin = 42
        plot_rect = rect.adjusted(left_margin, top_margin, -right_margin, -bottom_margin)
        if plot_rect.width() <= 0 or plot_rect.height() <= 0:
            return

        nominal_current_a = max(0.01, min(100.0, self._nominal_current_ma / 1000.0))
        i2t_threshold_a2s = max(1e-9, float(self._i2t_threshold)) / 1000000000.0

        # Amperage on X-axis (logarithmic, positive range only)
        x_min = 0.01
        x_max = max(10.0, nominal_current_a * 5.0)

        # Time on Y-axis (linear, 0 to 100 seconds)
        y_min = 0.0
        y_max = 100.0

        # Draw Vertical Grid Lines (at amperage decade intervals)
        painter.setPen(QPen(QColor("#2A2A2A"), 1))
        x_ticks = [0.01, 0.1, 0.5]
        decade = 1.0
        while decade <= x_max:
            for factor in (1.0, 2.0, 5.0):
                tick = decade * factor
                if tick >= 1.0 and tick <= x_max:
                    x_ticks.append(tick)
            decade *= 10.0
        x_ticks = sorted(set(x_ticks))

        for tick in x_ticks:
            x = self._log_map(tick, x_min, x_max, plot_rect.left(), plot_rect.right())
            painter.drawLine(int(x), plot_rect.top(), int(x), plot_rect.bottom())

        # Draw Horizontal Grid Lines (at time intervals)
        y_ticks = [i * 10.0 for i in range(int(y_max / 10.0) + 1)]
        y_ticks = [tick for tick in y_ticks if y_min <= tick <= y_max]

        for tick in y_ticks:
            y = self._linear_map(tick, y_min, y_max, plot_rect.bottom(), plot_rect.top())
            painter.drawLine(plot_rect.left(), int(y), plot_rect.right(), int(y))

        # Borders & Labels
        painter.setPen(QPen(QColor("#8A8A8A"), 1))
        painter.drawRect(plot_rect)
        painter.setPen(QColor("#B8B8B8"))
        painter.drawText(
            rect.adjusted(10, 2, -10, -2), Qt.AlignTop | Qt.AlignLeft, "Time (s)"
        )
        painter.drawText(
            rect.adjusted(10, 2, -10, -2),
            Qt.AlignBottom | Qt.AlignRight,
            "Overcurrent (A)",
        )

        # Y-axis labels (time)
        for tick in y_ticks:
            y = self._linear_map(tick, y_min, y_max, plot_rect.bottom(), plot_rect.top())
            painter.drawText(
                0, int(y) - 7, 45, 14, Qt.AlignRight | Qt.AlignVCenter, self._format_time_label(tick)
            )

        # X-axis labels (amperage)
        for tick in x_ticks:
            x = self._log_map(tick, x_min, x_max, plot_rect.left(), plot_rect.right())
            painter.drawText(
                int(x) - 28, plot_rect.bottom() + 14, 56, 16, Qt.AlignHCenter, self._format_current_label(tick)
            )

        # Plot the I2T protection curve
        curve_points = []
        sample_count = 200

        # Sample overcurrent values logarithmically for better coverage
        overcurrent_min = x_min
        overcurrent_max = min(max(x_max * 1.5, 100.0), 1000.0)

        for index in range(sample_count):
            ratio = index / max(1, sample_count - 1)
            overcurrent_a = overcurrent_min * ((overcurrent_max / overcurrent_min) ** ratio)

            if overcurrent_a < 0.01:
                continue

            denominator = (nominal_current_a + overcurrent_a) ** 2 - (nominal_current_a ** 2)
            if denominator <= 0:
                continue

            time_s = i2t_threshold_a2s / denominator

            if time_s < y_min or time_s > y_max:
                continue

            # Map: overcurrent_a to X logarithmically, time_s to Y linearly
            x = self._log_map(overcurrent_a, x_min, x_max, plot_rect.left(), plot_rect.right())
            y = self._linear_map(time_s, y_min, y_max, plot_rect.bottom(), plot_rect.top())

            curve_points.append(QPointF(float(x), float(y)))

        curve_points.sort(key=lambda pt: pt.x())

        if len(curve_points) >= 2:
            painter.setPen(QPen(QColor("#09BC8A" if self._enabled else "#6C8B82"), 2))
            painter.drawPolyline(QPolygonF(curve_points))

        painter.end()
