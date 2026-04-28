#include "shifttemplatedialog.h"

#include "databasemanager.h"

#include <QButtonGroup>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

namespace
{
QString assetPath(const QString &fileName)
{
    const QString candidate = QDir::cleanPath("C:/Users/Dmitrii/Documents/VKR_2/assets/" + fileName);
    return QFileInfo::exists(candidate) ? candidate : QString();
}

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

QCalendarWidget *createStyledCalendar(QWidget *parent)
{
    auto *calendar = new QCalendarWidget(parent);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar->setGridVisible(false);
    calendar->setStyleSheet(R"(
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            background: #FFFFFF;
            border-bottom: 1px solid #ECECF2;
            min-height: 36px;
        }
        QCalendarWidget QToolButton {
            color: #1C1D21;
            background: transparent;
            border: none;
            border-radius: 10px;
            min-width: 28px;
            min-height: 28px;
            font-size: 14px;
            font-weight: 600;
        }
        QCalendarWidget QToolButton:hover {
            background: #EEF2FF;
            color: #5E81F4;
        }
        QCalendarWidget QMenu {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 6px;
        }
        QCalendarWidget QSpinBox {
            background: transparent;
            border: none;
            color: #1C1D21;
            font-size: 14px;
            font-weight: 600;
        }
        QCalendarWidget QAbstractItemView:enabled {
            background: #FFFFFF;
            color: #1C1D21;
            selection-background-color: #5E81F4;
            selection-color: #FFFFFF;
            outline: 0;
            border: none;
            font-size: 13px;
        }
        QCalendarWidget QWidget {
            alternate-background-color: #FFFFFF;
        }
    )");
    return calendar;
}
}

ShiftTemplateDialog::ShiftTemplateDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    buildUi();
    applyStyles();
    loadSourceShifts();
    loadTemplates();
    updateTemplateDetails();
}

void ShiftTemplateDialog::buildUi()
{
    setWindowTitle("Шаблоны смен");
    resize(1040, 760);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto *titleLabel = new QLabel("Шаблоны смен", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Сохраняйте удачные смены как шаблоны и разворачивайте их на нужный период.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    auto *leftColumnLayout = new QVBoxLayout();
    leftColumnLayout->setSpacing(16);

    auto *sourceGroup = new QGroupBox("Сохранить шаблон из уже созданной смены", this);
    sourceGroup->setObjectName("cardGroupBox");
    auto *sourceLayout = new QVBoxLayout(sourceGroup);
    sourceLayout->setContentsMargins(18, 18, 18, 18);
    sourceLayout->setSpacing(12);

    auto *sourceHintLabel = new QLabel(
        "Выберите смену из истории и сохраните её как шаблон для повторного использования.",
        sourceGroup);
    sourceHintLabel->setObjectName("sectionHintLabel");
    sourceHintLabel->setWordWrap(true);

    sourceShiftListWidget = new QListWidget(sourceGroup);
    sourceShiftListWidget->setObjectName("styledListWidget");
    sourceShiftListWidget->setAlternatingRowColors(false);
    sourceShiftListWidget->setFocusPolicy(Qt::NoFocus);

    auto *saveTemplateButton = new QPushButton("Сохранить выбранную смену как шаблон", sourceGroup);
    saveTemplateButton->setObjectName("primaryButton");

    sourceLayout->addWidget(sourceHintLabel);
    sourceLayout->addWidget(sourceShiftListWidget, 1);
    sourceLayout->addWidget(saveTemplateButton);

    auto *templatesGroup = new QGroupBox("Список шаблонов", this);
    templatesGroup->setObjectName("cardGroupBox");
    auto *templatesLayout = new QVBoxLayout(templatesGroup);
    templatesLayout->setContentsMargins(18, 18, 18, 18);
    templatesLayout->setSpacing(12);

    templateListWidget = new QListWidget(templatesGroup);
    templateListWidget->setObjectName("styledListWidget");
    templateListWidget->setAlternatingRowColors(false);
    templateListWidget->setFocusPolicy(Qt::NoFocus);

    templateDetailsLabel = new QLabel("-", templatesGroup);
    templateDetailsLabel->setObjectName("detailsLabel");
    templateDetailsLabel->setWordWrap(true);
    templateDetailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *deleteTemplateButton = new QPushButton("Удалить шаблон", templatesGroup);
    deleteTemplateButton->setObjectName("dangerButton");

    templatesLayout->addWidget(templateListWidget, 1);
    templatesLayout->addWidget(templateDetailsLabel);
    templatesLayout->addWidget(deleteTemplateButton);

    leftColumnLayout->addWidget(sourceGroup, 1);
    leftColumnLayout->addWidget(templatesGroup, 1);

    auto *rightColumnLayout = new QVBoxLayout();
    rightColumnLayout->setSpacing(16);

    auto *applyGroup = new QGroupBox("Применить шаблон на период", this);
    applyGroup->setObjectName("cardGroupBox");
    auto *applyLayout = new QVBoxLayout(applyGroup);
    applyLayout->setContentsMargins(18, 18, 18, 18);
    applyLayout->setSpacing(14);

    auto *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setSpacing(12);

    periodStartEdit = new QDateEdit(QDate::currentDate(), applyGroup);
    periodStartEdit->setObjectName("styledDateEdit");
    periodStartEdit->setCalendarPopup(true);
    periodStartEdit->setDisplayFormat("dd.MM.yyyy");
    periodStartEdit->setCalendarWidget(createStyledCalendar(periodStartEdit));

    periodEndEdit = new QDateEdit(QDate::currentDate().addDays(6), applyGroup);
    periodEndEdit->setObjectName("styledDateEdit");
    periodEndEdit->setCalendarPopup(true);
    periodEndEdit->setDisplayFormat("dd.MM.yyyy");
    periodEndEdit->setCalendarWidget(createStyledCalendar(periodEndEdit));

    formLayout->addRow("Дата начала", periodStartEdit);
    formLayout->addRow("Дата окончания", periodEndEdit);

    auto *repeatModeGroup = new QGroupBox("Режим повторения", applyGroup);
    repeatModeGroup->setObjectName("innerGroupBox");
    auto *repeatModeLayout = new QVBoxLayout(repeatModeGroup);
    repeatModeLayout->setContentsMargins(14, 14, 14, 14);
    repeatModeLayout->setSpacing(10);

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

    auto *weekdayRowFrame = new QFrame(applyGroup);
    auto *weekdayLayout = new QHBoxLayout(weekdayRowFrame);
    weekdayLayout->setContentsMargins(0, 0, 0, 0);
    weekdayLayout->setSpacing(8);

    for (int day = 1; day <= 7; ++day)
    {
        auto *checkBox = new QCheckBox(weekdayLabel(day), applyGroup);
        checkBox->setChecked(day <= 5);
        weekdayCheckBoxes.append(checkBox);
        weekdayLayout->addWidget(checkBox);
    }
    weekdayLayout->addStretch();

    auto *hintLabel = new QLabel(
        "Если на выбранную дату уже есть смена с тем же временем начала и окончания, "
        "она будет пропущена, чтобы не создавать дубликаты.",
        applyGroup);
    hintLabel->setObjectName("sectionHintLabel");
    hintLabel->setWordWrap(true);

    auto *applyTemplateButton = new QPushButton("Применить выбранный шаблон", applyGroup);
    applyTemplateButton->setObjectName("primaryButton");

    applyLayout->addLayout(formLayout);
    applyLayout->addWidget(repeatModeGroup);
    applyLayout->addWidget(weekdayRowFrame);
    applyLayout->addWidget(hintLabel);
    applyLayout->addStretch();
    applyLayout->addWidget(applyTemplateButton);

    rightColumnLayout->addWidget(applyGroup);
    rightColumnLayout->addStretch();

    contentLayout->addLayout(leftColumnLayout, 3);
    contentLayout->addLayout(rightColumnLayout, 2);

    auto *closeButton = new QPushButton("Закрыть", this);
    closeButton->setObjectName("secondaryButton");

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

void ShiftTemplateDialog::applyStyles()
{
    const QString chevronPath = assetPath("chevron-down.svg").replace("\\", "/");
    setStyleSheet(QString(R"(
        QDialog {
            background: #F6F6F6;
        }
        QLabel#dialogTitleLabel {
            color: #1C1D21;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#dialogSubtitleLabel,
        QLabel#sectionHintLabel {
            color: #8181A5;
            font-size: 14px;
        }
        QGroupBox#cardGroupBox {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
            margin-top: 12px;
            color: #1C1D21;
            font-size: 15px;
            font-weight: 700;
        }
        QGroupBox#cardGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 6px;
        }
        QGroupBox#innerGroupBox {
            background: #FAFBFF;
            border: 1px solid #ECECF2;
            border-radius: 16px;
            margin-top: 10px;
            color: #1C1D21;
            font-size: 14px;
            font-weight: 700;
        }
        QGroupBox#innerGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 4px;
        }
        QListWidget#styledListWidget {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 16px;
            padding: 8px;
        }
        QListWidget#styledListWidget::item {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 12px;
            margin: 4px 0;
            color: #1C1D21;
        }
        QListWidget#styledListWidget::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QLabel#detailsLabel {
            background: #FAFBFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 12px;
            color: #1C1D21;
            font-size: 13px;
        }
        QDateEdit#styledDateEdit, QLineEdit {
            min-height: 40px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0 12px 0 12px;
            color: #1C1D21;
            font-size: 14px;
        }
        QDateEdit#styledDateEdit:focus, QLineEdit:focus {
            border: 1px solid #5E81F4;
        }
        QDateEdit#styledDateEdit::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 34px;
            border: none;
            border-left: 1px solid #ECECF2;
            background: #FAFBFF;
            border-top-right-radius: 12px;
            border-bottom-right-radius: 12px;
        }
        QDateEdit#styledDateEdit::drop-down:hover {
            background: #EEF2FF;
        }
        QDateEdit#styledDateEdit::down-arrow {
            image: url(%1);
            width: 12px;
            height: 8px;
        }
        QCheckBox {
            color: #1C1D21;
            font-size: 13px;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 6px;
            border: 1px solid #D6DBEE;
            background: #FFFFFF;
        }
        QCheckBox::indicator:checked {
            background: #5E81F4;
            border: 1px solid #5E81F4;
        }
        QPushButton {
            min-height: 42px;
            border: none;
            border-radius: 14px;
            padding: 0 16px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton#primaryButton {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton#primaryButton:hover {
            background: #4C6FE0;
        }
        QPushButton#secondaryButton {
            background: #E9EDFB;
            color: #5E81F4;
        }
        QPushButton#dangerButton {
            background: #FFF1F3;
            color: #FF808B;
            border: 1px solid #FFD8DE;
        }
        QLabel {
            color: #1C1D21;
        }
    )").arg(chevronPath));
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
        for (const ShiftAssignedEmployeeData &assignment : assignments)
            assignedLines << QString("%1 — %2").arg(assignment.positionName, assignment.employeeName);
        lines << QString("Назначенные: %1").arg(assignedLines.join("; "));
    }

    const QList<ShiftOpenPositionData> openPositions =
        DatabaseManager::instance().getShiftTemplateOpenPositions(templateId);
    if (!openPositions.isEmpty())
    {
        QStringList openLines;
        for (const ShiftOpenPositionData &openPosition : openPositions)
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
            dates.append(currentDate);
        else if (allowedDays.contains(currentDate.dayOfWeek()))
            dates.append(currentDate);
    }

    return dates;
}

QString ShiftTemplateDialog::requestTemplateName(const QString &defaultName, bool *accepted)
{
    bool localAccepted = false;

    QDialog dialog(this);
    dialog.setWindowTitle("Новый шаблон смены");
    dialog.setModal(true);
    dialog.resize(430, 220);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel("Новый шаблон смены", &dialog);
    titleLabel->setStyleSheet("color:#1C1D21;font-size:20px;font-weight:700;");

    auto *subtitleLabel = new QLabel("Введите название, под которым шаблон будет сохранён.", &dialog);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setStyleSheet("color:#8181A5;font-size:13px;");

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(defaultName);
    nameEdit->selectAll();

    auto *statusLabel = new QLabel(&dialog);
    statusLabel->setStyleSheet("color:#FF808B;font-size:12px;");
    statusLabel->clear();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Сохранить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(nameEdit);
    layout->addWidget(statusLabel);
    layout->addStretch();
    layout->addWidget(buttons);

    dialog.setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
        }
        QLineEdit {
            min-height: 40px;
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0 12px;
            color: #1C1D21;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #5E81F4;
        }
        QPushButton {
            min-height: 40px;
            border-radius: 12px;
            border: none;
            padding: 0 14px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton[text="Сохранить"] {
            background: #5E81F4;
            color: #FFFFFF;
        }
        QPushButton[text="Отмена"] {
            background: #E9EDFB;
            color: #5E81F4;
        }
    )");

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        if (nameEdit->text().trimmed().isEmpty())
        {
            statusLabel->setText("Введите название шаблона.");
            return;
        }
        localAccepted = true;
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged, &dialog, [statusLabel]() {
        statusLabel->clear();
    });

    dialog.exec();

    if (accepted)
        *accepted = localAccepted;
    return localAccepted ? nameEdit->text().trimmed() : QString();
}

void ShiftTemplateDialog::showStyledInformation(const QString &title, const QString &text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Information);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox { background: #F6F6F6; }
        QMessageBox QLabel { color: #1C1D21; font-size: 14px; }
        QMessageBox QLabel#qt_msgbox_label { font-size: 18px; font-weight: 700; }
        QMessageBox QLabel#qt_msgbox_informativelabel { color: #8181A5; font-size: 13px; min-width: 280px; }
        QMessageBox QPushButton {
            min-width: 120px; min-height: 40px; border-radius: 12px; border: none;
            padding: 0 14px; font-size: 14px; font-weight: 600;
            background: #5E81F4; color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

void ShiftTemplateDialog::showStyledWarning(const QString &title, const QString &text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox { background: #F6F6F6; }
        QMessageBox QLabel { color: #1C1D21; font-size: 14px; }
        QMessageBox QLabel#qt_msgbox_label { font-size: 18px; font-weight: 700; }
        QMessageBox QLabel#qt_msgbox_informativelabel { color: #8181A5; font-size: 13px; min-width: 280px; }
        QMessageBox QPushButton {
            min-width: 120px; min-height: 40px; border-radius: 12px; border: none;
            padding: 0 14px; font-size: 14px; font-weight: 600;
            background: #F4B85E; color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

void ShiftTemplateDialog::showStyledError(const QString &title, const QString &text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setText("Понятно");
    messageBox.setStyleSheet(R"(
        QMessageBox { background: #F6F6F6; }
        QMessageBox QLabel { color: #1C1D21; font-size: 14px; }
        QMessageBox QLabel#qt_msgbox_label { font-size: 18px; font-weight: 700; }
        QMessageBox QLabel#qt_msgbox_informativelabel { color: #8181A5; font-size: 13px; min-width: 280px; }
        QMessageBox QPushButton {
            min-width: 120px; min-height: 40px; border-radius: 12px; border: none;
            padding: 0 14px; font-size: 14px; font-weight: 600;
            background: #FF808B; color: #FFFFFF;
        }
    )");
    messageBox.exec();
}

QMessageBox::StandardButton ShiftTemplateDialog::showStyledQuestion(const QString &title, const QString &text)
{
    QMessageBox messageBox(this);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setWindowTitle(title);
    messageBox.setText(title);
    messageBox.setInformativeText(text);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::No);
    messageBox.button(QMessageBox::Yes)->setText("Да");
    messageBox.button(QMessageBox::No)->setText("Нет");
    messageBox.setStyleSheet(R"(
        QMessageBox { background: #F6F6F6; }
        QMessageBox QLabel { color: #1C1D21; font-size: 14px; }
        QMessageBox QLabel#qt_msgbox_label { font-size: 18px; font-weight: 700; }
        QMessageBox QLabel#qt_msgbox_informativelabel { color: #8181A5; font-size: 13px; min-width: 280px; }
        QMessageBox QPushButton {
            min-width: 110px; min-height: 40px; border-radius: 12px; border: none;
            padding: 0 14px; font-size: 14px; font-weight: 600;
        }
        QMessageBox QPushButton[text="Да"] {
            background: #5E81F4; color: #FFFFFF;
        }
        QMessageBox QPushButton[text="Нет"] {
            background: #E9EDFB; color: #5E81F4;
        }
    )");
    return static_cast<QMessageBox::StandardButton>(messageBox.exec());
}

void ShiftTemplateDialog::onSaveTemplateClicked()
{
    QListWidgetItem *item = currentSourceShiftItem();
    if (!item)
    {
        showStyledInformation("Шаблоны смен", "Сначала выберите смену, которую хотите сохранить как шаблон.");
        return;
    }

    bool accepted = false;
    const QString defaultName = QString("Шаблон %1").arg(item->text().section('|', 0, 0).trimmed());
    const QString name = requestTemplateName(defaultName, &accepted);

    if (!accepted || name.isEmpty())
        return;

    if (!DatabaseManager::instance().createShiftTemplateFromShift(
            currentBusinessId,
            item->data(Qt::UserRole).toInt(),
            name))
    {
        showStyledError("Ошибка", "Не удалось сохранить шаблон смены.");
        return;
    }

    loadTemplates();
    showStyledInformation("Шаблоны смен", "Шаблон сохранён.");
}

void ShiftTemplateDialog::onApplyTemplateClicked()
{
    QListWidgetItem *item = currentTemplateItem();
    if (!item)
    {
        showStyledInformation("Шаблоны смен", "Сначала выберите шаблон.");
        return;
    }

    if (periodStartEdit->date() > periodEndEdit->date())
    {
        showStyledWarning("Шаблоны смен", "Дата окончания не может быть раньше даты начала.");
        return;
    }

    const QList<QDate> dates = buildApplyDates();
    if (dates.isEmpty())
    {
        showStyledWarning("Шаблоны смен", "Не удалось сформировать даты применения. Проверьте период и режим повторения.");
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
        showStyledError("Ошибка", "Не удалось применить шаблон.");
        return;
    }

    showStyledInformation(
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
        showStyledInformation("Шаблоны смен", "Сначала выберите шаблон для удаления.");
        return;
    }

    if (showStyledQuestion("Удаление шаблона", QString("Удалить шаблон \"%1\"?").arg(item->text()))
        != QMessageBox::Yes)
    {
        return;
    }

    if (!DatabaseManager::instance().deleteShiftTemplate(item->data(Qt::UserRole).toInt()))
    {
        showStyledError("Ошибка", "Не удалось удалить шаблон.");
        return;
    }

    loadTemplates();
    updateTemplateDetails();
}
