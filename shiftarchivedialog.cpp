#include "shiftarchivedialog.h"

#include "databasemanager.h"

#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

ShiftArchiveDialog::ShiftArchiveDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    setWindowTitle("Архив смен");
    resize(900, 650);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    auto *titleLabel = new QLabel("Все смены предприятия", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    archiveListWidget = new QListWidget(this);
    archiveListWidget->setAlternatingRowColors(true);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(archiveListWidget, 1);

    loadArchive();
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
        lines << QString("%1 | %2 | %3")
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
}
