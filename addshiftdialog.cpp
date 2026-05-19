#include "addshiftdialog.h"

#include <QCoreApplication>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDir>
#include <QDoubleValidator>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTextEdit>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace
{
QString assetPath(const QString &fileName)
{
    const QStringList candidates = {
        QDir::currentPath() + "/assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../assets/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../../assets/" + fileName,
        "C:/Users/Dmitrii/Documents/VKR_2/assets/" + fileName
    };

    for (const QString &candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }

    return QString();
}

QString paymentSummary(const QString& paymentType,
                       const QString& hourlyRate,
                       const QString& fixedRate,
                       const QString& percentRate)
{
    if (paymentType == "Почасовая")
        return QString("почасовая: %1").arg(hourlyRate.isEmpty() ? "-" : hourlyRate);

    if (paymentType == "Фиксированная ставка")
        return QString("фиксированная: %1").arg(fixedRate.isEmpty() ? "-" : fixedRate);

    if (paymentType == "Процент")
        return QString("процент: %1").arg(percentRate.isEmpty() ? "-" : percentRate);

    return QString("ставка: %1, процент: %2")
        .arg(fixedRate.isEmpty() ? "-" : fixedRate,
             percentRate.isEmpty() ? "-" : percentRate);
}

void styleShiftMessageBox(QMessageBox& box, QMessageBox::Icon icon, const QString& title, const QString& text)
{
    Q_UNUSED(icon);

    box.setWindowTitle(title);
    box.setIcon(QMessageBox::NoIcon);
    box.setText(title);
    box.setInformativeText(text);
    box.setStandardButtons(QMessageBox::Ok);
    if (box.button(QMessageBox::Ok))
        box.button(QMessageBox::Ok)->setText("Понятно");
    box.setStyleSheet(R"(
        QMessageBox {
            background: #F6F6FB;
        }
        QMessageBox QLabel {
            color: #1C1D21;
            font-size: 14px;
            min-width: 260px;
            max-width: 320px;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox QLabel#qt_msgbox_label {
            font-size: 18px;
            font-weight: 700;
            color: #1C1D21;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox QLabel#qt_msgbox_informativelabel {
            color: #8181A5;
            font-size: 13px;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox QPushButton {
            min-width: 120px;
            min-height: 40px;
            border-radius: 12px;
            background: #5E81F4;
            color: white;
            border: none;
            font-weight: 600;
            padding: 0 16px;
        }
        QMessageBox QPushButton:hover {
            background: #4E73EB;
        }
    )");

    for (QLabel *label : box.findChildren<QLabel*>())
    {
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setMinimumWidth(260);
        label->setMaximumWidth(320);
    }
}

void showShiftWarning(QWidget *parent, const QString& title, const QString& text)
{
    QMessageBox box(parent);
    styleShiftMessageBox(box, QMessageBox::Warning, title, text);
    box.exec();
}

void showShiftError(QWidget *parent, const QString& title, const QString& text)
{
    QMessageBox box(parent);
    styleShiftMessageBox(box, QMessageBox::Critical, title, text);
    box.exec();
}
}

AddShiftDialog::AddShiftDialog(int businessId, int shiftId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
    , currentShiftId(shiftId)
{
    buildUi();
    loadEmployees();
    loadPositions();

    if (currentShiftId > 0)
        loadShift();
}

bool AddShiftDialog::hasOpenPositions() const
{
    return !openPositions.isEmpty();
}

int AddShiftDialog::savedShiftId() const
{
    return lastSavedShiftId;
}

void AddShiftDialog::buildUi()
{
    const bool editMode = currentShiftId > 0;

    setWindowTitle(editMode ? "Редактирование смены" : "Создание смены");
    resize(940, 700);
    setMinimumSize(820, 540);
    setSizeGripEnabled(true);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *contentWidget = new QWidget(scrollArea);
    auto *contentWrapperLayout = new QVBoxLayout(contentWidget);
    contentWrapperLayout->setContentsMargins(0, 0, 0, 0);
    contentWrapperLayout->setSpacing(18);

    auto *titleLabel = new QLabel(editMode ? "Редактирование смены" : "Новая смена", this);
    titleLabel->setObjectName("dialogTitleLabel");

    auto *subtitleLabel = new QLabel(
        "Заполните параметры смены, назначьте сотрудников и добавьте свободные позиции для отклика через VK.",
        this);
    subtitleLabel->setObjectName("dialogSubtitleLabel");
    subtitleLabel->setWordWrap(true);

    auto *baseFrame = new QFrame(this);
    baseFrame->setObjectName("sectionCard");
    auto *baseLayout = new QFormLayout(baseFrame);
    baseLayout->setContentsMargins(18, 18, 18, 18);
    baseLayout->setHorizontalSpacing(14);
    baseLayout->setVerticalSpacing(12);

    shiftDateEdit = new QDateEdit(this);
    shiftDateEdit->setCalendarPopup(true);
    shiftDateEdit->setDisplayFormat("dd.MM.yyyy");
    shiftDateEdit->setDate(QDate::currentDate());

    startTimeEdit = new QTimeEdit(this);
    startTimeEdit->setDisplayFormat("HH:mm");
    startTimeEdit->setTime(QTime(9, 0));
    startTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);

    endTimeEdit = new QTimeEdit(this);
    endTimeEdit->setDisplayFormat("HH:mm");
    endTimeEdit->setTime(QTime(18, 0));
    endTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);

    statusComboBox = new QComboBox(this);
    statusComboBox->addItems({"Запланирована", "Выполнена", "Отменена"});

    commentEdit = new QTextEdit(this);
    commentEdit->setMinimumHeight(84);
    commentEdit->setMaximumHeight(120);

    createdAtEdit = new QLineEdit(this);
    createdAtEdit->setReadOnly(true);
    createdAtEdit->setText(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));

    baseLayout->addRow("Дата смены", shiftDateEdit);
    baseLayout->addRow("Время начала", startTimeEdit);
    baseLayout->addRow("Время окончания", endTimeEdit);
    baseLayout->addRow("Статус", statusComboBox);
    baseLayout->addRow("Комментарий", commentEdit);
    baseLayout->addRow("Дата создания", createdAtEdit);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(18);

    auto *amountValidator = new QDoubleValidator(0.0, 1000000000.0, 2, this);
    amountValidator->setNotation(QDoubleValidator::StandardNotation);
    auto *percentValidator = new QDoubleValidator(0.0, 100.0, 2, this);
    percentValidator->setNotation(QDoubleValidator::StandardNotation);

    auto *assignedFrame = new QFrame(this);
    assignedFrame->setObjectName("sectionCard");
    auto *assignedFrameLayout = new QVBoxLayout(assignedFrame);
    assignedFrameLayout->setContentsMargins(18, 18, 18, 18);
    assignedFrameLayout->setSpacing(12);

    auto *assignedTitle = new QLabel("Назначенные сотрудники", assignedFrame);
    assignedTitle->setObjectName("sectionTitleLabel");

    auto *assignedForm = new QFormLayout();
    assignedForm->setHorizontalSpacing(12);
    assignedForm->setVerticalSpacing(10);

    assignedPositionComboBox = new QComboBox(this);
    assignedEmployeeComboBox = new QComboBox(this);
    assignedPaymentTypeComboBox = new QComboBox(this);
    assignedPaymentTypeComboBox->addItems(
        {"Почасовая", "Фиксированная ставка", "Процент", "Ставка + процент"});
    assignedHourlyRateEdit = new QLineEdit(this);
    assignedFixedRateEdit = new QLineEdit(this);
    assignedPercentRateEdit = new QLineEdit(this);
    assignedHourlyRateEdit->setValidator(amountValidator);
    assignedFixedRateEdit->setValidator(amountValidator);
    assignedPercentRateEdit->setValidator(percentValidator);
    assignedHourlyRateEdit->setPlaceholderText("Ставка в час");
    assignedFixedRateEdit->setPlaceholderText("Сумма за смену");
    assignedPercentRateEdit->setPlaceholderText("Процент числом");

    assignedForm->addRow("Должность", assignedPositionComboBox);
    assignedForm->addRow("Сотрудник", assignedEmployeeComboBox);
    assignedForm->addRow("Тип оплаты", assignedPaymentTypeComboBox);
    assignedForm->addRow("Почасовая ставка", assignedHourlyRateEdit);
    assignedForm->addRow("Фиксированная ставка", assignedFixedRateEdit);
    assignedForm->addRow("Процент", assignedPercentRateEdit);

    auto *assignedButtonsLayout = new QHBoxLayout();
    auto *addAssignedButton = new QPushButton("Добавить сотрудника", this);
    addAssignedButton->setObjectName("secondaryButton");
    removeAssignedButton = new QPushButton("Удалить из смены", this);
    removeAssignedButton->setObjectName("ghostButton");
    assignedButtonsLayout->addWidget(addAssignedButton);
    assignedButtonsLayout->addWidget(removeAssignedButton);

    assignedListWidget = new QListWidget(this);
    assignedListWidget->setAlternatingRowColors(false);
    assignedListWidget->setFocusPolicy(Qt::NoFocus);
    assignedListWidget->setSpacing(8);
    assignedListWidget->setMinimumHeight(170);

    assignedFrameLayout->addWidget(assignedTitle);
    assignedFrameLayout->addLayout(assignedForm);
    assignedFrameLayout->addLayout(assignedButtonsLayout);
    assignedFrameLayout->addWidget(assignedListWidget, 1);

    auto *openFrame = new QFrame(this);
    openFrame->setObjectName("sectionCard");
    auto *openFrameLayout = new QVBoxLayout(openFrame);
    openFrameLayout->setContentsMargins(18, 18, 18, 18);
    openFrameLayout->setSpacing(12);

    auto *openTitle = new QLabel("Свободные позиции", openFrame);
    openTitle->setObjectName("sectionTitleLabel");

    auto *openForm = new QFormLayout();
    openForm->setHorizontalSpacing(12);
    openForm->setVerticalSpacing(10);

    openPositionComboBox = new QComboBox(this);
    openCountSpinBox = new QSpinBox(this);
    openCountSpinBox->setMinimum(1);
    openCountSpinBox->setMaximum(100);
    openCountSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    openPaymentTypeComboBox = new QComboBox(this);
    openPaymentTypeComboBox->addItems(
        {"Почасовая", "Фиксированная ставка", "Процент", "Ставка + процент"});
    openHourlyRateEdit = new QLineEdit(this);
    openFixedRateEdit = new QLineEdit(this);
    openPercentRateEdit = new QLineEdit(this);
    openHourlyRateEdit->setValidator(amountValidator);
    openFixedRateEdit->setValidator(amountValidator);
    openPercentRateEdit->setValidator(percentValidator);
    openHourlyRateEdit->setPlaceholderText("Ставка в час");
    openFixedRateEdit->setPlaceholderText("Сумма за смену");
    openPercentRateEdit->setPlaceholderText("Процент числом");

    openForm->addRow("Должность", openPositionComboBox);
    openForm->addRow("Количество", openCountSpinBox);
    openForm->addRow("Тип оплаты", openPaymentTypeComboBox);
    openForm->addRow("Почасовая ставка", openHourlyRateEdit);
    openForm->addRow("Фиксированная ставка", openFixedRateEdit);
    openForm->addRow("Процент", openPercentRateEdit);

    auto *openButtonsLayout = new QHBoxLayout();
    auto *addOpenPositionButton = new QPushButton("Добавить позицию", this);
    addOpenPositionButton->setObjectName("secondaryButton");
    removeOpenPositionButton = new QPushButton("Удалить позицию", this);
    removeOpenPositionButton->setObjectName("ghostButton");
    openButtonsLayout->addWidget(addOpenPositionButton);
    openButtonsLayout->addWidget(removeOpenPositionButton);

    openListWidget = new QListWidget(this);
    openListWidget->setAlternatingRowColors(false);
    openListWidget->setFocusPolicy(Qt::NoFocus);
    openListWidget->setSpacing(8);
    openListWidget->setMinimumHeight(170);

    openFrameLayout->addWidget(openTitle);
    openFrameLayout->addLayout(openForm);
    openFrameLayout->addLayout(openButtonsLayout);
    openFrameLayout->addWidget(openListWidget, 1);

    contentLayout->addWidget(assignedFrame, 1);
    contentLayout->addWidget(openFrame, 1);

    auto *saveButton = new QPushButton(editMode ? "Сохранить изменения" : "Сохранить смену", this);
    saveButton->setObjectName("primaryButton");

    connect(assignedPaymentTypeComboBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        updatePaymentFields(
            assignedPaymentTypeComboBox,
            assignedHourlyRateEdit,
            assignedFixedRateEdit,
            assignedPercentRateEdit);
    });
    connect(openPaymentTypeComboBox, &QComboBox::currentTextChanged, this, [this](const QString&) {
        updatePaymentFields(
            openPaymentTypeComboBox,
            openHourlyRateEdit,
            openFixedRateEdit,
            openPercentRateEdit);
    });
    connect(addAssignedButton, &QPushButton::clicked, this, [this]() {
        addAssignedEmployee();
    });
    connect(removeAssignedButton, &QPushButton::clicked, this, [this]() {
        removeAssignedEmployee();
    });
    connect(addOpenPositionButton, &QPushButton::clicked, this, [this]() {
        addOpenPosition();
    });
    connect(removeOpenPositionButton, &QPushButton::clicked, this, [this]() {
        removeOpenPosition();
    });
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        saveShift();
    });

    contentWrapperLayout->addWidget(titleLabel);
    contentWrapperLayout->addWidget(subtitleLabel);
    contentWrapperLayout->addWidget(baseFrame);
    contentWrapperLayout->addLayout(contentLayout, 1);
    contentWrapperLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);

    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(saveButton);

    const QString chevronPath = assetPath("chevron-down.svg").replace("\\", "/");
    setStyleSheet(QString(R"(
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
        QFrame#sectionCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLineEdit, QComboBox, QSpinBox, QDateEdit, QTimeEdit, QTextEdit {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            padding: 10px 14px;
            color: #1C1D21;
            font-size: 14px;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
        QTextEdit {
            padding-top: 12px;
            padding-bottom: 12px;
        }
        QListWidget {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 14px;
            outline: 0;
            padding: 6px;
        }
        QListWidget::item {
            background: #F9FAFF;
            border: 1px solid #E6EAF8;
            border-radius: 14px;
            margin: 4px 0px;
            padding: 12px 14px;
            color: #1C1D21;
        }
        QListWidget::item:selected {
            background: #EEF2FF;
            border: 1px solid #5E81F4;
            color: #1C1D21;
        }
        QComboBox::drop-down, QDateEdit::drop-down {
            width: 28px;
            border: none;
            subcontrol-origin: padding;
            subcontrol-position: top right;
        }
        QComboBox::down-arrow, QDateEdit::down-arrow {
            image: url(%1);
            width: 12px;
            height: 12px;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 24px;
            border: none;
            background: transparent;
            subcontrol-origin: border;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: #F3F5FC;
            border-radius: 8px;
        }
        QSpinBox::up-arrow {
            image: none;
            width: 0px;
            height: 0px;
        }
        QSpinBox::down-arrow {
            image: url(%1);
            width: 10px;
            height: 10px;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 12px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #D7E2FF;
            min-height: 46px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #C1D2FF;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0px;
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
        QPushButton#secondaryButton:hover,
        QPushButton#ghostButton:hover {
            background: #E3EAFE;
        }
        QPushButton#ghostButton {
            background: #F3F4F8;
            color: #8181A5;
        }
        QPushButton:disabled {
            background: #EEF0F6;
            color: #A8ADBD;
            border: 1px solid #E1E5F0;
        }
        QLineEdit:disabled,
        QComboBox:disabled,
        QSpinBox:disabled,
        QDateEdit:disabled,
        QTimeEdit:disabled,
        QTextEdit:disabled {
            background: #F3F4F8;
            color: #A8ADBD;
            border: 1px solid #E1E5F0;
        }
        QLineEdit:read-only,
        QTextEdit:read-only {
            background: #F6F7FB;
            color: #8E94A6;
            border: 1px solid #E4E8F2;
        }
        QListWidget:disabled {
            background: #F3F4F8;
            color: #A8ADBD;
            border: 1px solid #E1E5F0;
        }
    )").arg(chevronPath));

    updatePaymentFields(
        assignedPaymentTypeComboBox,
        assignedHourlyRateEdit,
        assignedFixedRateEdit,
        assignedPercentRateEdit);
    updatePaymentFields(
        openPaymentTypeComboBox,
        openHourlyRateEdit,
        openFixedRateEdit,
        openPercentRateEdit);
}

void AddShiftDialog::loadEmployees()
{
    assignedEmployeeComboBox->clear();

    QSqlQuery query = DatabaseManager::instance().getEmployees(currentBusinessId, true);
    while (query.next())
        assignedEmployeeComboBox->addItem(query.value("full_name").toString(), query.value("id"));
}

void AddShiftDialog::loadPositions()
{
    assignedPositionComboBox->clear();
    openPositionComboBox->clear();

    const QStringList positions = DatabaseManager::instance().getPositionNames(currentBusinessId);
    for (const QString& position : positions)
    {
        assignedPositionComboBox->addItem(position);
        openPositionComboBox->addItem(position);
    }
}

void AddShiftDialog::loadShift()
{
    QSqlQuery query = DatabaseManager::instance().getShiftById(currentShiftId);
    if (!query.next())
        return;

    shiftDateEdit->setDate(QDate::fromString(query.value("shift_date").toString(), Qt::ISODate));
    startTimeEdit->setTime(QTime::fromString(query.value("start_time").toString(), "HH:mm"));
    endTimeEdit->setTime(QTime::fromString(query.value("end_time").toString(), "HH:mm"));

    const QString status = query.value("status").toString();
    const int statusIndex = statusComboBox->findText(status);
    if (statusIndex >= 0)
        statusComboBox->setCurrentIndex(statusIndex);

    commentEdit->setPlainText(query.value("comment").toString());

    const QString createdAt = query.value("created_at").toString();
    QDateTime createdAtDateTime = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!createdAtDateTime.isValid())
        createdAtDateTime = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss");
    createdAtEdit->setText(createdAtDateTime.isValid()
                               ? createdAtDateTime.toString("dd.MM.yyyy HH:mm")
                               : createdAt);

    assignedEmployees = DatabaseManager::instance().getShiftAssignments(currentShiftId);
    openPositions = DatabaseManager::instance().getShiftOpenPositions(currentShiftId);
    refreshAssignedList();
    refreshOpenPositionsList();
}

void AddShiftDialog::updatePaymentFields(QComboBox *paymentTypeComboBox,
                                         QLineEdit *hourlyRateEdit,
                                         QLineEdit *fixedRateEdit,
                                         QLineEdit *percentRateEdit)
{
    const QString paymentType = paymentTypeComboBox->currentText();

    const bool needHourly = paymentType == "Почасовая";
    const bool needFixed = paymentType == "Фиксированная ставка" || paymentType == "Ставка + процент";
    const bool needPercent = paymentType == "Процент" || paymentType == "Ставка + процент";

    hourlyRateEdit->setEnabled(needHourly);
    fixedRateEdit->setEnabled(needFixed);
    percentRateEdit->setEnabled(needPercent);

    if (!needHourly)
        hourlyRateEdit->clear();
    if (!needFixed)
        fixedRateEdit->clear();
    if (!needPercent)
        percentRateEdit->clear();
}

void AddShiftDialog::refreshAssignedList()
{
    assignedListWidget->clear();

    for (int i = 0; i < assignedEmployees.size(); ++i)
    {
        const ShiftAssignedEmployeeData& item = assignedEmployees.at(i);
        auto *listItem = new QListWidgetItem(
            QString("%1 — %2 | %3")
                .arg(item.positionName,
                     item.employeeName,
                     paymentSummary(item.paymentType, item.hourlyRate, item.fixedRate, item.percentRate)),
            assignedListWidget);
        listItem->setData(Qt::UserRole, i);
    }
}

void AddShiftDialog::refreshOpenPositionsList()
{
    openListWidget->clear();

    for (int i = 0; i < openPositions.size(); ++i)
    {
        const ShiftOpenPositionData& item = openPositions.at(i);
        auto *listItem = new QListWidgetItem(
            QString("%1 — %2 чел. | %3")
                .arg(item.positionName)
                .arg(item.employeeCount)
                .arg(paymentSummary(item.paymentType, item.hourlyRate, item.fixedRate, item.percentRate)),
            openListWidget);
        listItem->setData(Qt::UserRole, i);
    }
}

void AddShiftDialog::addAssignedEmployee()
{
    if (assignedPositionComboBox->currentText().trimmed().isEmpty()
        || assignedEmployeeComboBox->currentText().trimmed().isEmpty())
    {
        showShiftWarning(this, "Ошибка", "Выберите должность и сотрудника.");
        return;
    }

    const int selectedEmployeeId = assignedEmployeeComboBox->currentData().toInt();
    for (const ShiftAssignedEmployeeData &assignment : assignedEmployees)
    {
        if (assignment.employeeId == selectedEmployeeId)
        {
            showShiftWarning(this, "Ошибка", "Этот сотрудник уже назначен на данную смену.");
            return;
        }
    }

    ShiftAssignedEmployeeData item;
    item.employeeId = selectedEmployeeId;
    item.employeeName = assignedEmployeeComboBox->currentText();
    item.positionName = assignedPositionComboBox->currentText();
    item.paymentType = assignedPaymentTypeComboBox->currentText();
    item.hourlyRate = assignedHourlyRateEdit->text().trimmed();
    item.fixedRate = assignedFixedRateEdit->text().trimmed();
    item.percentRate = assignedPercentRateEdit->text().trimmed();

    assignedEmployees.append(item);
    refreshAssignedList();
}

void AddShiftDialog::removeAssignedEmployee()
{
    QListWidgetItem *item = assignedListWidget->currentItem();
    if (!item)
        return;

    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= assignedEmployees.size())
        return;

    assignedEmployees.removeAt(index);
    refreshAssignedList();
}

void AddShiftDialog::addOpenPosition()
{
    if (openPositionComboBox->currentText().trimmed().isEmpty())
    {
        showShiftWarning(this, "Ошибка", "Выберите должность для свободной позиции.");
        return;
    }

    ShiftOpenPositionData item;
    item.positionName = openPositionComboBox->currentText();
    item.employeeCount = openCountSpinBox->value();
    item.paymentType = openPaymentTypeComboBox->currentText();
    item.hourlyRate = openHourlyRateEdit->text().trimmed();
    item.fixedRate = openFixedRateEdit->text().trimmed();
    item.percentRate = openPercentRateEdit->text().trimmed();

    openPositions.append(item);
    refreshOpenPositionsList();
}

void AddShiftDialog::removeOpenPosition()
{
    QListWidgetItem *item = openListWidget->currentItem();
    if (!item)
        return;

    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= openPositions.size())
        return;

    openPositions.removeAt(index);
    refreshOpenPositionsList();
}

void AddShiftDialog::saveShift()
{
    if (startTimeEdit->time() >= endTimeEdit->time())
    {
        showShiftWarning(this, "Ошибка", "Время окончания должно быть позже времени начала.");
        return;
    }

    if (assignedEmployees.isEmpty() && openPositions.isEmpty())
    {
        showShiftWarning(
            this,
            "Ошибка",
            "Добавьте хотя бы одного назначенного сотрудника или одну свободную позицию.");
        return;
    }

    for (int i = 0; i < assignedEmployees.size(); ++i)
    {
        for (int j = i + 1; j < assignedEmployees.size(); ++j)
        {
            if (assignedEmployees.at(i).employeeId == assignedEmployees.at(j).employeeId)
            {
                showShiftWarning(this, "Ошибка", "Один сотрудник не может быть назначен на одну смену несколько раз.");
                return;
            }
        }
    }

    bool ok = false;
    if (currentShiftId > 0)
    {
        ok = DatabaseManager::instance().updateShift(
            currentShiftId,
            shiftDateEdit->date(),
            startTimeEdit->time(),
            endTimeEdit->time(),
            statusComboBox->currentText(),
            commentEdit->toPlainText(),
            assignedEmployees,
            openPositions);
    }
    else
    {
        ok = DatabaseManager::instance().createShift(
            currentBusinessId,
            shiftDateEdit->date(),
            startTimeEdit->time(),
            endTimeEdit->time(),
            statusComboBox->currentText(),
            commentEdit->toPlainText(),
            assignedEmployees,
            openPositions);
    }

    if (!ok)
    {
        showShiftError(
            this,
            "Ошибка",
            currentShiftId > 0
                ? "Не удалось сохранить изменения смены."
                : "Не удалось сохранить смену в базе данных.");
        return;
    }

    lastSavedShiftId = currentShiftId > 0
        ? currentShiftId
        : DatabaseManager::instance().getLastShiftIdForBusiness(currentBusinessId);

    accept();
}
