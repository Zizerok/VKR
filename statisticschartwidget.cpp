#include "statisticschartwidget.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>

StatisticsChartWidget::StatisticsChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(280);
    setAutoFillBackground(true);
}

void StatisticsChartWidget::setBarData(const QString& title,
                                       const QStringList& categories,
                                       const QList<double>& values,
                                       const QColor& color,
                                       const QString& emptyText)
{
    mode = Mode::Bar;
    chartTitle = title;
    chartEmptyText = emptyText;
    barCategories = categories;
    barValues = values;
    barColor = color;
    pieValues.clear();
    pieColors.clear();
    update();
}

void StatisticsChartWidget::setPieData(const QString& title,
                                       const QList<QPair<QString, double>>& values,
                                       const QList<QColor>& colors,
                                       const QString& emptyText)
{
    mode = Mode::Pie;
    chartTitle = title;
    chartEmptyText = emptyText;
    pieValues = values;
    pieColors = colors;
    barCategories.clear();
    barValues.clear();
    update();
}

void StatisticsChartWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect fullRect = rect().adjusted(8, 8, -8, -8);
    painter.fillRect(fullRect, QColor(255, 255, 255));
    painter.setPen(QColor(220, 226, 232));
    painter.drawRoundedRect(fullRect, 10, 10);

    QRect titleRect = fullRect.adjusted(14, 10, -14, 0);
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    painter.setFont(titleFont);
    painter.setPen(QColor(31, 41, 55));
    painter.drawText(titleRect, Qt::AlignLeft | Qt::TextWordWrap, chartTitle);

    QRect contentRect = fullRect.adjusted(14, 44, -14, -14);

    if ((mode == Mode::Bar && (barCategories.isEmpty() || barValues.isEmpty()))
        || (mode == Mode::Pie && pieValues.isEmpty())
        || mode == Mode::Empty)
    {
        drawEmptyState(painter, contentRect);
        return;
    }

    if (mode == Mode::Bar)
        drawBarChart(painter, contentRect);
    else
        drawPieChart(painter, contentRect);
}

void StatisticsChartWidget::drawBarChart(QPainter& painter, const QRect& rect)
{
    const int itemCount = std::min(barCategories.size(), barValues.size());
    if (itemCount <= 0)
    {
        drawEmptyState(painter, rect);
        return;
    }

    double maxValue = 0.0;
    for (int index = 0; index < itemCount; ++index)
        maxValue = std::max(maxValue, barValues.at(index));
    if (maxValue <= 0.0)
        maxValue = 1.0;

    const int labelHeight = 42;
    const QRect chartRect = rect.adjusted(8, 8, -8, -labelHeight);

    painter.setPen(QColor(209, 213, 219));
    painter.drawLine(chartRect.bottomLeft(), chartRect.bottomRight());

    const int gap = 12;
    const int barWidth = std::max(24, (chartRect.width() - gap * (itemCount - 1)) / std::max(1, itemCount));

    for (int index = 0; index < itemCount; ++index)
    {
        const double value = barValues.at(index);
        const int height = qRound((value / maxValue) * std::max(20, chartRect.height() - 24));
        const int x = chartRect.left() + index * (barWidth + gap);
        const QRect barRect(x, chartRect.bottom() - height, barWidth, height);

        painter.fillRect(barRect, barColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(barRect, 4, 4);

        painter.setPen(QColor(55, 65, 81));
        painter.drawText(QRect(x - 8, chartRect.top(), barWidth + 16, chartRect.height() - height - 4),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         QString::number(value, 'f', 0));

        painter.drawText(QRect(x - 10, chartRect.bottom() + 8, barWidth + 20, labelHeight - 8),
                         Qt::AlignHCenter | Qt::TextWordWrap,
                         barCategories.at(index));
    }
}

void StatisticsChartWidget::drawPieChart(QPainter& painter, const QRect& rect)
{
    double total = 0.0;
    for (const auto& item : pieValues)
        total += std::max(0.0, item.second);

    if (total <= 0.0)
    {
        drawEmptyState(painter, rect);
        return;
    }

    const int legendWidth = std::min(220, rect.width() / 2);
    const QRect pieRect = QRect(rect.left(), rect.top(), rect.width() - legendWidth - 12, rect.height()).adjusted(12, 8, -12, -8);
    const int diameter = std::min(pieRect.width(), pieRect.height());
    const QRect circleRect(pieRect.left() + (pieRect.width() - diameter) / 2,
                           pieRect.top() + (pieRect.height() - diameter) / 2,
                           diameter,
                           diameter);

    int startAngle = 0;
    for (int index = 0; index < pieValues.size(); ++index)
    {
        const double value = std::max(0.0, pieValues.at(index).second);
        if (value <= 0.0)
            continue;

        const int spanAngle = qRound(5760.0 * value / total);
        painter.setBrush(index < pieColors.size() ? pieColors.at(index) : QColor::fromHsv((index * 45) % 360, 180, 220));
        painter.setPen(Qt::NoPen);
        painter.drawPie(circleRect, startAngle, spanAngle);
        startAngle += spanAngle;
    }

    const QRect legendRect(rect.right() - legendWidth, rect.top() + 12, legendWidth, rect.height() - 24);
    int legendY = legendRect.top();
    painter.setPen(QColor(31, 41, 55));

    for (int index = 0; index < pieValues.size(); ++index)
    {
        const double value = std::max(0.0, pieValues.at(index).second);
        if (value <= 0.0)
            continue;

        const QColor color = index < pieColors.size() ? pieColors.at(index) : QColor::fromHsv((index * 45) % 360, 180, 220);
        painter.fillRect(QRect(legendRect.left(), legendY + 4, 14, 14), color);
        painter.drawText(QRect(legendRect.left() + 22, legendY, legendRect.width() - 22, 22),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QString("%1 — %2").arg(pieValues.at(index).first).arg(value, 0, 'f', 0));
        legendY += 24;
    }
}

void StatisticsChartWidget::drawEmptyState(QPainter& painter, const QRect& rect)
{
    painter.setPen(QColor(107, 114, 128));
    painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, chartEmptyText);
}
