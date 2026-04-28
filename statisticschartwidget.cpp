#include "statisticschartwidget.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>

StatisticsChartWidget::StatisticsChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(360);
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
    painter.drawRoundedRect(fullRect, 14, 14);

    QRect titleRect = fullRect.adjusted(18, 14, -18, 0);
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 3);
    painter.setFont(titleFont);
    painter.setPen(QColor(31, 41, 55));
    painter.drawText(titleRect, Qt::AlignLeft | Qt::TextWordWrap, chartTitle);

    QRect contentRect = fullRect.adjusted(18, 62, -18, -18);

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

    const int labelHeight = 58;
    const QRect chartRect = rect.adjusted(8, 8, -8, -labelHeight);

    painter.setPen(QColor(209, 213, 219));
    painter.drawLine(chartRect.bottomLeft(), chartRect.bottomRight());

    const int gap = 14;
    const int barWidth = std::max(28, (chartRect.width() - gap * (itemCount - 1)) / std::max(1, itemCount));

    for (int index = 0; index < itemCount; ++index)
    {
        const double value = barValues.at(index);
        const int height = qRound((value / maxValue) * std::max(20, chartRect.height() - 28));
        const int x = chartRect.left() + index * (barWidth + gap);
        const QRect barRect(x, chartRect.bottom() - height, barWidth, height);

        painter.fillRect(barRect, barColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(barRect, 6, 6);

        QFont valueFont = painter.font();
        valueFont.setPointSize(std::max(11, valueFont.pointSize()));
        painter.setFont(valueFont);
        painter.setPen(QColor(55, 65, 81));
        painter.drawText(QRect(x - 8, chartRect.top(), barWidth + 16, chartRect.height() - height - 6),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         QString::number(value, 'f', 0));

        QFont labelFont = painter.font();
        labelFont.setPointSize(std::max(10, labelFont.pointSize() - 1));
        painter.setFont(labelFont);
        painter.drawText(QRect(x - 12, chartRect.bottom() + 10, barWidth + 24, labelHeight - 10),
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

    const int legendWidth = std::min(300, rect.width() / 2);
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
    QFont legendFont = painter.font();
    legendFont.setPointSize(std::max(11, legendFont.pointSize()));
    painter.setFont(legendFont);

    for (int index = 0; index < pieValues.size(); ++index)
    {
        const double value = std::max(0.0, pieValues.at(index).second);
        if (value <= 0.0)
            continue;

        const QColor color = index < pieColors.size() ? pieColors.at(index) : QColor::fromHsv((index * 45) % 360, 180, 220);
        painter.fillRect(QRect(legendRect.left(), legendY + 5, 14, 14), color);
        painter.drawText(QRect(legendRect.left() + 24, legendY, legendRect.width() - 24, 34),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                         QString("%1 - %2").arg(pieValues.at(index).first).arg(value, 0, 'f', 0));
        legendY += 36;
    }
}

void StatisticsChartWidget::drawEmptyState(QPainter& painter, const QRect& rect)
{
    QFont emptyFont = painter.font();
    emptyFont.setPointSize(std::max(12, emptyFont.pointSize() + 1));
    painter.setFont(emptyFont);
    painter.setPen(QColor(107, 114, 128));
    painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, chartEmptyText);
}
