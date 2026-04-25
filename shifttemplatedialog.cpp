#include "shifttemplatedialog.h"

#include "databasemanager.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

namespace
{
QString weekdayLabel(int dayOfWeek)
{
    switch (dayOfWeek)
    {
    case 1: return "Пн";
    case 2: return "Вт";
    case 3: return "Ср";
    case 4: return "Чт";
    case 5: return "Пт";
    case 6: return "Сб";
    case 7: return "Вс";
    default: return "?";
    }
}
}

ShiftTemplateDialog::ShiftTemplateDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    buildUi();
    loadSourceShifts();
    loadTemplates();
    updateTemplateDetails();
}

void ShiftTemplateDialog::buildUi()
{
    setWindowTitle("Шаблоны смен");
    resize(980, 720);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    auto *titleLabel = new QLabel("Шаблоны смен", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *subtitleLabel = new QLabel(
        "Сохраняйте удачные смены как шаблоны и разворачивайте их на нужный период.",
        this);
    subtitleLabel->setWordWrap(true);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    auto *leftColumnLayout = new QVBoxLayout();
    leftColumnLayout->setSpacing(12);

    auto *sourceGroup = new QGroupBox("Сохранить шаблон из уже созданной смены", this);
    auto *sourceLayout = new QVBoxLayout(sourceGroup);
    sourceShiftListWidget = new QListWidget(sourceGroup);
    sourceShiftListWidget->setAlternatingRowColors(true);
    auto *saveTemplateButton = new QPushButton("Сохранить выбранную смену как шаблон", sourceGroup);
    sourceLayout->addWidget(sourceShiftListWidget, 1);
    sourceLayout->addWidget(saveTemplateButton);

    auto *templatesGroup = new QGroupBox("Список шаблонов", this);
    auto *templatesLayout = new QVBoxLayout(templatesGroup);
    templateListWidget = new QListWidget(templatesGroup);
    templateListWidget->setAlternatingRowColors(true);
    templateDetailsLabel = new QLabel("-", templatesGroup);
    templateDetailsLabel->setWordWrap(true);
    templateDetailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *deleteTemplateButton = new QPushButton("Удалить шаблон", templatesGroup);
    templatesLayout->addWidget(templateListWidget, 1);
    templatesLayout->addWidget(templateDetailsLabel);
    templatesLayout->addWidget(deleteTemplateButton);

    leftColumnLayout->addWidget(sourceGroup, 1);
    leftColumnLayout->addWidget(templatesGroup, 1);

    auto *rightColumnLayout = new QVBoxLayout();
    rightColumnLayout->setSpacing(12);

    auto *applyGroup = new QGroupBox("Применить шаблон на период", this);
    auto *applyLayout = new QVBoxLayout(applyGroup);
    auto *formLayout = new QFormLayout();

    periodStartEdit = new QDateEdit(QDate::currentDate(), applyGroup);
    periodStartEdit->setCalendarPopup(true);
    periodStartEdit->setDisplayFormat("dd.MM.yyyy");

    periodEndEdit = new QDateEdit(QDate::currentDate().addDays(6), applyGroup);
    periodEndEdit->setCalendarPopup(true);
    periodEndEdit->setDisplayFormat("dd.MM.yyyy");

    formLayout->addRow("Дата начала:", periodStartEdit);
    formLayout->addRow("Дата окончания:", periodEndEdit);

    auto *repeatModeGroup = new QGroupBox("Режим повторения", applyGroup);
    auto *repeatModeLayout = new QVBoxLayout(repeatModeGroup);
    repeatGroup = new QButtonGroup(this);

    auto *singleCheck = new QCheckBox("Разово: только дата начала", repeatModeGroup);
    auto *dailyCheck = new QCheckBox("Каждый день", repeatModeGroup);
    auto *weekdayCheck = new QCheckBox("Только выбранные дни недели", repeatModeGroup);

    repeatGroup->setExclusive(true);
    repeatGroup->addButton(singleCheck, 0);
    repeatGroup->addButton(dailyCheck, 1);
    repeatGroup->addButton(weekdayCheck, 2);
    dailyCheck->setChecked(true);

    repeatModeLayout->addWidget(singleCheck);
    repeatModeLayout->addWidget(dailyCheck);
    repeatModeLayout->addWidget(weekdayCheck);

    auto *weekdayLayout = new QHBoxLayout();
    for (int day = 1; day <= 7; ++day)
    {
        auto *checkBox = new QCheckBox(weekdayLabel(day), applyGroup);
        checkBox->setChecked(day <= 5);
        weekdayCheckBoxes.append(checkBox);
        weekdayLayout->addWidget(checkBox);
    }

    auto *hintLabel = new QLabel(
        "Если на выбранную дату уже есть смена с тем же временем начала и окончания, "
        "она будет пропущена, чтобы не плодить дубликаты.",
        applyGroup);
    hintLabel->setWordWrap(true);

    auto *applyTemplateButton = new QPushButton("Применить выбранный шаблон", applyGroup);

    applyLayout->addLayout(formLayout);
    applyLayout->addWidget(repeatModeGroup);
    applyLayout->addLayout(weekdayLayout);
    applyLayout->addWidget(hintLabel);
    applyLayout->addStretch();
    applyLayout->addWidget(applyTemplateButton);

    rightColumnLayout->addWidget(applyGroup);
    rightColumnLayout->addStretch();

    contentLayout->addLayout(leftColumnLayout, 3);
    contentLayout->addLayout(rightColumnLayout, 2);

    auto *closeButton = new QPushButton("Закрыть", this);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(contentLayout, 1);
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);

    connect(saveTemplateButton, &QPushButton::clicked, this, &ShiftTemplateDialog::onSaveTemplateClicked);
    connect(applyTemplateButton, &QPushButton::clicked, this, &ShiftTemplateDialog::onApplyTemplateClicked);
    connect(deleteTemplateButton, &QPushButton::clicked, this, &ShiftTemplateDialog::onDeleteTemplateClicked);
    connect(templateListWidget, &QListWidget::currentRowChanged, this, [this]() {
        updateTemplateDetails();
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ShiftTemplateDialog::loadSourceShifts()
{
    sourceShiftListWidget->clear();

    QSqlQuery query = DatabaseManager::instance().getAllShifts(currentBusinessId);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString line = QString("%1 | %2-%3 | %4")
                                 .arg(shiftDate.toString("dd.MM.yyyy"),
                                      query.value("start_time").toString(),
                                      query.value("end_time").toString(),
                                      query.value("status").toString());
        auto *item = new QListWidgetItem(line);
        item->setData(Qt::UserRole, shiftId);
        sourceShiftListWidget->addItem(item);
    }
}

void ShiftTemplateDialog::loadTemplates()
{
    templateListWidget->clear();

    QSqlQuery query = DatabaseManager::instance().getShiftTemplates(currentBusinessId);
    while (query.next())
    {
        auto *item = new QListWidgetItem(query.value("name").toString());
        item->setData(Qt::UserRole, query.value("id").toInt());
        templateListWidget->addItem(item);
    }

    if (templateListWidget->count() > 0 && !templateListWidget->currentItem())
        templateListWidget->setCurrentRow(0);
}

void ShiftTemplateDialog::updateTemplateDetails()
{
    QListWidgetItem *item = currentTemplateItem();
    if (!item)
    {
        templateDetailsLabel->setText("Выберите шаблон, чтобы увидеть его состав.");
        return;
    }

    const int templateId = item->data(Qt::UserRole).toInt();
    QSqlQuery query = DatabaseManager::instance().getShiftTemplateById(templateId);
    if (!query.next())
    {
        templateDetailsLabel->setText("Не удалось загрузить шаблон.");
        return;
    }

    QStringList lines;
    lines << QString("Время: %1 - %2")
                 .arg(query.value("start_time").toString(), query.value("end_time").toString());
    lines << QString("Статус: %1").arg(query.value("status").toString());

    const QString comment = query.value("comment").toString().trimmed();
    if (!comment.isEmpty())
        lines << QString("Комментарий: %1").arg(comment);

    const QList<ShiftAssignedEmployeeData> assignments =
        DatabaseManager::instance().getShiftTemplateAssignments(templateId);
    if (!assignments.isEmpty())
    {
        QStringList assignedLines;
        for (const ShiftAssignedEmployeeData& assignment : assignments)
            assignedLines << QString("%1 — %2").arg(assignment.positionName, assignment.employeeName);
        lines << QString("Назначенные: %1").arg(assignedLines.join("; "));
    }

    const QList<ShiftOpenPositionData> openPositions =
        DatabaseManager::instance().getShiftTemplateOpenPositions(templateId);
    if (!openPositions.isEmpty())
    {
        QStringList openLines;
        for (const ShiftOpenPositionData& openPosition : openPositions)
            openLines << QString("%1 x%2").arg(openPosition.positionName).arg(openPosition.employeeCount);
        lines << QString("Свободные позиции: %1").arg(openLines.join("; "));
    }

    templateDetailsLabel->setText(lines.join("\n"));
}

QListWidgetItem *ShiftTemplateDialog::currentSourceShiftItem() const
{
    return sourceShiftListWidget->currentItem();
}

QListWidgetItem *ShiftTemplateDialog::currentTemplateItem() const
{
    return templateListWidget->currentItem();
}

QList<QDate> ShiftTemplateDialog::buildApplyDates() const
{
    QList<QDate> dates;

    const QDate startDate = periodStartEdit->date();
    const QDate endDate = periodEndEdit->date();
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate)
        return dates;

    const int repeatMode = repeatGroup->checkedId();
    if (repeatMode == 0)
    {
        dates.append(startDate);
        return dates;
    }

    QSet<int> allowedDays;
    if (repeatMode == 2)
    {
        for (int index = 0; index < weekdayCheckBoxes.size(); ++index)
        {
            if (weekdayCheckBoxes.at(index)->isChecked())
                allowedDays.insert(index + 1);
        }

        if (allowedDays.isEmpty())
            return dates;
    }

    for (QDate currentDate = startDate; currentDate <= endDate; currentDate = currentDate.addDays(1))
    {
        if (repeatMode == 1)
        {
            dates.append(currentDate);
        }
        else if (allowedDays.contains(currentDate.dayOfWeek()))
        {
            dates.append(currentDate);
        }
    }

    return dates;
}

void ShiftTemplateDialog::onSaveTemplateClicked()
{
    QListWidgetItem *item = currentSourceShiftItem();
    if (!item)
    {
        QMessageBox::information(this, "Шаблоны смен", "Сначала выберите смену, которую хотите сохранить как шаблон.");
        return;
    }

    bool ok = false;
    const QString defaultName = QString("Шаблон %1").arg(item->text().section('|', 0, 0).trimmed());
    const QString name = QInputDialog::getText(
        this,
        "Новый шаблон смены",
        "Название шаблона:",
        QLineEdit::Normal,
        defaultName,
        &ok).trimmed();

    if (!ok || name.isEmpty())
        return;

    if (!DatabaseManager::instance().createShiftTemplateFromShift(
            currentBusinessId,
            item->data(Qt::UserRole).toInt(),
            name))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить шаблон смены.");
        return;
    }

    loadTemplates();
    QMessageBox::information(this, "Шаблоны смен", "Шаблон сохранён.");
}

void ShiftTemplateDialog::onApplyTemplateClicked()
{
    QListWidgetItem *item = currentTemplateItem();
    if (!item)
    {
        QMessageBox::information(this, "Шаблоны смен", "Сначала выберите шаблон.");
        return;
    }

    if (periodStartEdit->date() > periodEndEdit->date())
    {
        QMessageBox::warning(this, "Шаблоны смен", "Дата окончания не может быть раньше даты начала.");
        return;
    }

    const QList<QDate> dates = buildApplyDates();
    if (dates.isEmpty())
    {
        QMessageBox::warning(this, "Шаблоны смен", "Не удалось сформировать даты применения. Проверьте период и режим повторения.");
        return;
    }

    int createdCount = 0;
    int skippedCount = 0;
    if (!DatabaseManager::instance().applyShiftTemplate(
            item->data(Qt::UserRole).toInt(),
            dates,
            &createdCount,
            &skippedCount))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось применить шаблон.");
        return;
    }

    QMessageBox::information(
        this,
        "Шаблоны смен",
        QString("Готово.\nСоздано смен: %1\nПропущено как дубликаты: %2")
            .arg(createdCount)
            .arg(skippedCount));

    accept();
}

void ShiftTemplateDialog::onDeleteTemplateClicked()
{
    QListWidgetItem *item = currentTemplateItem();
    if (!item)
    {
        QMessageBox::information(this, "Шаблоны смен", "Сначала выберите шаблон для удаления.");
        return;
    }

    const auto reply = QMessageBox::question(
        this,
        "Удаление шаблона",
        QString("Удалить шаблон \"%1\"?").arg(item->text()));

    if (reply != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deleteShiftTemplate(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить шаблон.");
        return;
    }

    loadTemplates();
    updateTemplateDetails();
}
