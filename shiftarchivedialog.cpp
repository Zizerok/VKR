#include "shiftarchivedialog.h"

#include "databasemanager.h"

#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

ShiftArchiveDialog::ShiftArchiveDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    buildUi();
    loadArchive();
}

void ShiftArchiveDialog::buildUi()
{
    setWindowTitle("Архив смен");
    resize(980, 720);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Архив смен", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Здесь собраны все смены предприятия, включая завершённые и старые записи.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *listCard = new QFrame(this);
    listCard->setObjectName("listCard");
    auto *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(18, 18, 18, 18);
    listLayout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout();
    auto *headerLabel = new QLabel("Все смены", this);
    headerLabel->setObjectName("sectionTitleLabel");
    auto *closeButton = new QPushButton("Закрыть", this);
    closeButton->setObjectName("primaryButton");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeButton);

    archiveListWidget = new QListWidget(this);
    archiveListWidget->setAlternatingRowColors(false);
    archiveListWidget->setFocusPolicy(Qt::NoFocus);
    archiveListWidget->setSpacing(10);

    listLayout->addLayout(headerLayout);
    listLayout->addWidget(archiveListWidget, 1);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
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
        }
        QLabel#sectionTitleLabel {
            color: #1C1D21;
            font-size: 16px;
            font-weight: 700;
        }
        QFrame#listCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: 0;
            padding: 4px;
        }
        QListWidget::item {
            background: #F9FAFF;
            border: 1px solid #E6EAF8;
            border-radius: 16px;
            margin: 4px 0;
            padding: 14px 16px;
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
    )");

    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ShiftArchiveDialog::loadArchive()
{
    archiveListWidget->clear();

    QSqlQuery query = DatabaseManager::instance().getAllShifts(currentBusinessId);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString timeRange = QString("%1 - %2")
                                      .arg(query.value("start_time").toString(),
                                           query.value("end_time").toString());
        const QString status = query.value("status").toString();
        const QString comment = query.value("comment").toString().trimmed();

        QStringList lines;
        lines << QString("%1  •  %2  •  %3")
                     .arg(shiftDate.toString("dd.MM.yyyy"), timeRange, status);

        const QStringList assignedSummary = DatabaseManager::instance().getShiftAssignedSummary(shiftId);
        const QStringList openSummary = DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId);

        lines << QString("Назначены: %1")
                     .arg(assignedSummary.isEmpty() ? "-" : assignedSummary.join("; "));
        lines << QString("Свободные позиции: %1")
                     .arg(openSummary.isEmpty() ? "-" : openSummary.join("; "));

        if (!comment.isEmpty())
            lines << QString("Комментарий: %1").arg(comment);

        archiveListWidget->addItem(lines.join("\n"));
    }

    if (archiveListWidget->count() == 0)
    {
        auto *emptyItem = new QListWidgetItem("В архиве пока нет сохранённых смен.");
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        archiveListWidget->addItem(emptyItem);
    }
}
