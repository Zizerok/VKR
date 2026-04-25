#ifndef STATISTICSCHARTWIDGET_H
#define STATISTICSCHARTWIDGET_H

#include <QColor>
#include <QList>
#include <QString>
#include <QWidget>

class StatisticsChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsChartWidget(QWidget *parent = nullptr);

    void setBarData(const QString& title,
                    const QStringList& categories,
                    const QList<double>& values,
                    const QColor& color,
                    const QString& emptyText = QString("Нет данных за выбранный период"));

    void setPieData(const QString& title,
                    const QList<QPair<QString, double>>& values,
                    const QList<QColor>& colors,
                    const QString& emptyText = QString("Нет данных за выбранный период"));

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum class Mode
    {
        Empty,
        Bar,
        Pie
    };

    Mode mode = Mode::Empty;
    QString chartTitle;
    QString chartEmptyText = "Нет данных за выбранный период";
    QStringList barCategories;
    QList<double> barValues;
    QColor barColor = QColor(59, 130, 246);
    QList<QPair<QString, double>> pieValues;
    QList<QColor> pieColors;

    void drawBarChart(QPainter& painter, const QRect& rect);
    void drawPieChart(QPainter& painter, const QRect& rect);
    void drawEmptyState(QPainter& painter, const QRect& rect);
};

#endif // STATISTICSCHARTWIDGET_H
