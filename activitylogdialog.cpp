#include "activitylogdialog.h"

#include "databasemanager.h"

#include <QComboBox>
#include <QDateTime>
#include <QFrame>
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

    return dateTime.isValid() ? dateTime.toString("dd.MM.yyyy HH:mm") : value;
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
    resize(920, 680);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Журнал действий", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *descriptionLabel = new QLabel(
        "Здесь хранится история ключевых действий: сотрудники, смены, выплаты, уведомления, шаблоны и события VK.",
        this);
    descriptionLabel->setObjectName("dialogSubtitleLabel");
    descriptionLabel->setWordWrap(true);

    auto *filterCard = new QFrame(this);
    filterCard->setObjectName("controlsCard");
    auto *controlsLayout = new QHBoxLayout(filterCard);
    controlsLayout->setContentsMargins(18, 16, 18, 16);
    controlsLayout->setSpacing(12);

    auto *filterLabel = new QLabel("Фильтр", this);
    filterLabel->setObjectName("fieldLabel");

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
    refreshButton->setObjectName("secondaryButton");
    auto *closeButton = new QPushButton("Закрыть", this);
    closeButton->setObjectName("primaryButton");

    controlsLayout->addWidget(filterLabel);
    controlsLayout->addWidget(filterComboBox, 1);
    controlsLayout->addWidget(refreshButton);
    controlsLayout->addWidget(closeButton);

    auto *listCard = new QFrame(this);
    listCard->setObjectName("listCard");
    auto *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(18, 18, 18, 18);
    listLayout->setSpacing(12);

    logListWidget = new QListWidget(this);
    logListWidget->setAlternatingRowColors(false);
    logListWidget->setFocusPolicy(Qt::NoFocus);
    logListWidget->setSpacing(10);

    listLayout->addWidget(logListWidget);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(filterCard);
    mainLayout->addWidget(listCard, 1);

    setStyleSheet(R"(
        QDialog {
            background: #F6F6FB;
        }
        QLabel#dialogTitleLabel {
            color: #1C1D21;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#dialogSubtitleLabel {
            color: #8181A5;
            font-size: 13px;
            line-height: 1.4;
        }
        QLabel#fieldLabel {
            color: #8181A5;
            font-size: 13px;
            font-weight: 600;
        }
        QFrame#controlsCard, QFrame#listCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QComboBox, QListWidget {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 10px 14px;
            color: #1C1D21;
            font-size: 14px;
        }
        QComboBox::drop-down {
            width: 28px;
            border: none;
        }
        QComboBox::down-arrow {
            image: none;
        }
        QListWidget {
            outline: 0;
            padding: 8px;
        }
        QListWidget::item {
            background: #F9FAFF;
            border: 1px solid #E6EAF8;
            border-radius: 16px;
            margin: 4px 0px;
            padding: 12px 14px;
            color: #1C1D21;
        }
        QListWidget::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QPushButton {
            min-height: 42px;
            border-radius: 14px;
            padding: 0 18px;
            font-size: 14px;
            font-weight: 600;
            border: none;
        }
        QPushButton#primaryButton {
            background: #5E81F4;
            color: white;
        }
        QPushButton#primaryButton:hover {
            background: #4E73EB;
        }
        QPushButton#secondaryButton {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QPushButton#secondaryButton:hover {
            background: #E3EAFE;
        }
    )");

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
        auto *emptyItem = new QListWidgetItem("Пока журнал действий пуст.");
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        logListWidget->addItem(emptyItem);
        return;
    }

    for (const ActivityLogInfo& log : logs)
    {
        QStringList lines;
        lines << QString("%1  •  %2").arg(formatLogDate(log.createdAt), entityTypeLabel(log.entityType));
        lines << log.description;
        if (log.relatedShiftId > 0)
            lines << QString("Смена ID: %1").arg(log.relatedShiftId);
        if (log.relatedEmployeeId > 0)
            lines << QString("Сотрудник ID: %1").arg(log.relatedEmployeeId);

        logListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}
