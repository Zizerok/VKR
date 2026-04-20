#include "addshiftdialog.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace
{
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
}

AddShiftDialog::AddShiftDialog(int businessId, QWidget *parent)
    : QDialog(parent)
    , currentBusinessId(businessId)
{
    buildUi();
    loadEmployees();
    loadPositions();
}

void AddShiftDialog::buildUi()
{
    setWindowTitle("Создание смены");
    resize(980, 860);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(18);

    auto *titleLabel = new QLabel("Новая смена", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *baseFrame = new QFrame(this);
    auto *baseLayout = new QFormLayout(baseFrame);
    baseLayout->setHorizontalSpacing(14);
    baseLayout->setVerticalSpacing(12);

    shiftDateEdit = new QDateEdit(this);
    shiftDateEdit->setCalendarPopup(true);
    shiftDateEdit->setDisplayFormat("dd.MM.yyyy");
    shiftDateEdit->setDate(QDate::currentDate());

    startTimeEdit = new QTimeEdit(this);
    startTimeEdit->setDisplayFormat("HH:mm");
    startTimeEdit->setTime(QTime(9, 0));

    endTimeEdit = new QTimeEdit(this);
    endTimeEdit->setDisplayFormat("HH:mm");
    endTimeEdit->setTime(QTime(18, 0));

    statusComboBox = new QComboBox(this);
    statusComboBox->addItems({"Запланирована", "Выполнена", "Отменена"});

    commentEdit = new QTextEdit(this);
    commentEdit->setMinimumHeight(90);

    baseLayout->addRow("Дата смены:", shiftDateEdit);
    baseLayout->addRow("Время начала:", startTimeEdit);
    baseLayout->addRow("Время окончания:", endTimeEdit);
    baseLayout->addRow("Статус:", statusComboBox);
    baseLayout->addRow("Комментарий:", commentEdit);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(18);

    auto *assignedFrame = new QFrame(this);
    auto *assignedFrameLayout = new QVBoxLayout(assignedFrame);
    assignedFrameLayout->setSpacing(12);

    auto *assignedTitle = new QLabel("Назначенные сотрудники", assignedFrame);
    QFont sectionFont = assignedTitle->font();
    sectionFont.setPointSize(13);
    sectionFont.setBold(true);
    assignedTitle->setFont(sectionFont);

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

    assignedHourlyRateEdit->setPlaceholderText("Ставка в час");
    assignedFixedRateEdit->setPlaceholderText("Сумма за смену");
    assignedPercentRateEdit->setPlaceholderText("Процент");

    assignedForm->addRow("Должность:", assignedPositionComboBox);
    assignedForm->addRow("Сотрудник:", assignedEmployeeComboBox);
    assignedForm->addRow("Тип оплаты:", assignedPaymentTypeComboBox);
    assignedForm->addRow("Почасовая ставка:", assignedHourlyRateEdit);
    assignedForm->addRow("Фиксированная ставка:", assignedFixedRateEdit);
    assignedForm->addRow("Процент:", assignedPercentRateEdit);

    auto *assignedButtonsLayout = new QHBoxLayout();
    auto *addAssignedButton = new QPushButton("Добавить сотрудника", this);
    removeAssignedButton = new QPushButton("Удалить из смены", this);
    assignedButtonsLayout->addWidget(addAssignedButton);
    assignedButtonsLayout->addWidget(removeAssignedButton);

    assignedListWidget = new QListWidget(this);
    assignedListWidget->setAlternatingRowColors(true);

    assignedFrameLayout->addWidget(assignedTitle);
    assignedFrameLayout->addLayout(assignedForm);
    assignedFrameLayout->addLayout(assignedButtonsLayout);
    assignedFrameLayout->addWidget(assignedListWidget, 1);

    auto *openFrame = new QFrame(this);
    auto *openFrameLayout = new QVBoxLayout(openFrame);
    openFrameLayout->setSpacing(12);

    auto *openTitle = new QLabel("Свободные позиции", openFrame);
    openTitle->setFont(sectionFont);

    auto *openForm = new QFormLayout();
    openForm->setHorizontalSpacing(12);
    openForm->setVerticalSpacing(10);

    openPositionComboBox = new QComboBox(this);
    openCountSpinBox = new QSpinBox(this);
    openCountSpinBox->setMinimum(1);
    openCountSpinBox->setMaximum(100);
    openPaymentTypeComboBox = new QComboBox(this);
    openPaymentTypeComboBox->addItems(
        {"Почасовая", "Фиксированная ставка", "Процент", "Ставка + процент"});
    openHourlyRateEdit = new QLineEdit(this);
    openFixedRateEdit = new QLineEdit(this);
    openPercentRateEdit = new QLineEdit(this);

    openHourlyRateEdit->setPlaceholderText("Ставка в час");
    openFixedRateEdit->setPlaceholderText("Сумма за смену");
    openPercentRateEdit->setPlaceholderText("Процент");

    openForm->addRow("Должность:", openPositionComboBox);
    openForm->addRow("Количество:", openCountSpinBox);
    openForm->addRow("Тип оплаты:", openPaymentTypeComboBox);
    openForm->addRow("Почасовая ставка:", openHourlyRateEdit);
    openForm->addRow("Фиксированная ставка:", openFixedRateEdit);
    openForm->addRow("Процент:", openPercentRateEdit);

    auto *openButtonsLayout = new QHBoxLayout();
    auto *addOpenPositionButton = new QPushButton("Добавить позицию", this);
    removeOpenPositionButton = new QPushButton("Удалить позицию", this);
    openButtonsLayout->addWidget(addOpenPositionButton);
    openButtonsLayout->addWidget(removeOpenPositionButton);

    openListWidget = new QListWidget(this);
    openListWidget->setAlternatingRowColors(true);

    openFrameLayout->addWidget(openTitle);
    openFrameLayout->addLayout(openForm);
    openFrameLayout->addLayout(openButtonsLayout);
    openFrameLayout->addWidget(openListWidget, 1);

    contentLayout->addWidget(assignedFrame, 1);
    contentLayout->addWidget(openFrame, 1);

    auto *saveButton = new QPushButton("Сохранить смену", this);

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

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(baseFrame);
    mainLayout->addLayout(contentLayout, 1);
    mainLayout->addWidget(saveButton);

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
                .arg(item.positionName, item.employeeName,
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
        QMessageBox::warning(this, "Ошибка", "Выберите должность и сотрудника.");
        return;
    }

    ShiftAssignedEmployeeData item;
    item.employeeId = assignedEmployeeComboBox->currentData().toInt();
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
        QMessageBox::warning(this, "Ошибка", "Выберите должность для свободной позиции.");
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
        QMessageBox::warning(this, "Ошибка", "Время окончания должно быть позже времени начала.");
        return;
    }

    if (assignedEmployees.isEmpty() && openPositions.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Ошибка",
            "Добавьте хотя бы одного назначенного сотрудника или одну свободную позицию.");
        return;
    }

    const bool ok = DatabaseManager::instance().createShift(
        currentBusinessId,
        shiftDateEdit->date(),
        startTimeEdit->time(),
        endTimeEdit->time(),
        statusComboBox->currentText(),
        commentEdit->toPlainText(),
        assignedEmployees,
        openPositions);

    if (!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить смену в базу данных.");
        return;
    }

    accept();
}
