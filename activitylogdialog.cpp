#include "activitylogdialog.h"

#include "databasemanager.h"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
QString formatLogDate(const QString& value)
{
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");

    return dateTime.isValid()
        ? dateTime.toString("dd.MM.yyyy HH:mm")
        : value;
}

QString entityTypeLabel(const QString& entityType)
{
    if (entityType == "employee")
        return "Сотрудники";
    if (entityType == "position")
        return "Должности";
    if (entityType == "shift")
        return "Смены";
    if (entityType == "payment")
        return "Выплаты";
    if (entityType == "notification")
        return "Уведомления";
    if (entityType == "template")
        return "Шаблоны";
    if (entityType == "vk")
        return "VK";
    return "Другое";
}
}

ActivityLogDialog::ActivityLogDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    buildUi();
    loadLogs();
}

void ActivityLogDialog::buildUi()
{
    setWindowTitle("Журнал действий");
    resize(860, 620);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    auto *titleLabel = new QLabel("Бизнес-лог системы", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *descriptionLabel = new QLabel(
        "Здесь хранится история ключевых действий: сотрудники, смены, выплаты, уведомления, шаблоны и VK-события.",
        this);
    descriptionLabel->setWordWrap(true);

    auto *controlsLayout = new QHBoxLayout();
    filterComboBox = new QComboBox(this);
    filterComboBox->addItem("Все события", "");
    filterComboBox->addItem("Сотрудники", "employee");
    filterComboBox->addItem("Должности", "position");
    filterComboBox->addItem("Смены", "shift");
    filterComboBox->addItem("Выплаты", "payment");
    filterComboBox->addItem("Уведомления", "notification");
    filterComboBox->addItem("Шаблоны", "template");
    filterComboBox->addItem("VK", "vk");

    auto *refreshButton = new QPushButton("Обновить", this);
    auto *closeButton = new QPushButton("Закрыть", this);

    controlsLayout->addWidget(new QLabel("Фильтр:", this));
    controlsLayout->addWidget(filterComboBox);
    controlsLayout->addWidget(refreshButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(closeButton);

    logListWidget = new QListWidget(this);
    logListWidget->setAlternatingRowColors(true);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addLayout(controlsLayout);
    mainLayout->addWidget(logListWidget, 1);

    connect(filterComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        loadLogs();
    });
    connect(refreshButton, &QPushButton::clicked, this, &ActivityLogDialog::loadLogs);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ActivityLogDialog::loadLogs()
{
    if (!logListWidget)
        return;

    logListWidget->clear();

    const QString filter = filterComboBox ? filterComboBox->currentData().toString() : QString();
    const QList<ActivityLogInfo> logs = DatabaseManager::instance().getActivityLogs(currentBusinessId, filter);

    if (logs.isEmpty())
    {
        logListWidget->addItem("Пока нет записей журнала.");
        return;
    }

    for (const ActivityLogInfo& log : logs)
    {
        QStringList lines;
        lines << QString("%1 | %2").arg(formatLogDate(log.createdAt), entityTypeLabel(log.entityType));
        lines << log.description;
        if (log.relatedShiftId > 0)
            lines << QString("Смена ID: %1").arg(log.relatedShiftId);
        if (log.relatedEmployeeId > 0)
            lines << QString("Сотрудник ID: %1").arg(log.relatedEmployeeId);

        logListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}
