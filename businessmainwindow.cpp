#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

#include "addemployeedialog.h"
#include "addshiftdialog.h"
#include "employeecardwindow.h"
#include "positioneditdialog.h"

#include <QColor>
#include <QDoubleValidator>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
double parsePaymentNumber(const QString& text)
{
    bool ok = false;
    const double value = QString(text).trimmed().replace(',', '.').toDouble(&ok);
    return ok ? value : 0.0;
}

double calculatePaymentAmount(const ShiftPaymentInfo& payment)
{
    if (payment.paymentType == "Почасовая")
    {
        const QTime start = QTime::fromString(payment.timeRange.section(" - ", 0, 0), "HH:mm");
        const QTime end = QTime::fromString(payment.timeRange.section(" - ", 1, 1), "HH:mm");
        const double hours = start.isValid() && end.isValid() ? start.secsTo(end) / 3600.0 : 0.0;
        return hours * parsePaymentNumber(payment.hourlyRate);
    }

    if (payment.paymentType == "Фиксированная ставка")
        return parsePaymentNumber(payment.fixedRate);

    if (payment.paymentType == "Ставка + процент")
        return parsePaymentNumber(payment.fixedRate)
            + parsePaymentNumber(payment.revenueAmount) * parsePaymentNumber(payment.percentRate) / 100.0;

    if (!payment.percentRate.trimmed().isEmpty())
    {
        const double percentPart = parsePaymentNumber(payment.revenueAmount)
            * parsePaymentNumber(payment.percentRate) / 100.0;
        if (!payment.fixedRate.trimmed().isEmpty())
            return parsePaymentNumber(payment.fixedRate) + percentPart;
        return percentPart;
    }

    return 0.0;
}
}

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    setupNavigation();
    setupShiftsSection();
    setupStaffSection();
    setupPaymentsSection();
    showSection(0, "Смены");
}

BusinessMainWindow::BusinessMainWindow(int currentUserId, int businessId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    Q_UNUSED(currentUserId);
    currentBusinessId = businessId;

    setupNavigation();
    setupShiftsSection();
    setupStaffSection();
    setupPaymentsSection();

    ui->labelBusinessTitle->setText(DatabaseManager::instance().getBusinessName(businessId));
    showSection(0, "Смены");
}

BusinessMainWindow::~BusinessMainWindow()
{
    delete ui;
}

void BusinessMainWindow::setupNavigation()
{
    connect(ui->pushButtonShifts, &QPushButton::clicked, this, [this]() {
        showSection(0, "Смены");
    });
    connect(ui->pushButtonStaff, &QPushButton::clicked, this, [this]() {
        showSection(1, "Персонал");
    });
    connect(ui->pushButtonPayments, &QPushButton::clicked, this, [this]() {
        showSection(2, "Выплаты");
    });
    connect(ui->pushButtonCommunication, &QPushButton::clicked, this, [this]() {
        showSection(3, "Коммуникация");
    });
    connect(ui->pushButtonSettings, &QPushButton::clicked, this, [this]() {
        showSection(4, "Настройки");
    });
}

void BusinessMainWindow::showSection(int index, const QString& sectionTitle)
{
    ui->stackedWidgetSections->setCurrentIndex(index);
    ui->labelCurrentSection->setText(sectionTitle);

    if (index == 0)
        ui->pushButtonShifts->setChecked(true);
    else if (index == 1)
        ui->pushButtonStaff->setChecked(true);
    else if (index == 2)
        ui->pushButtonPayments->setChecked(true);
    else if (index == 3)
        ui->pushButtonCommunication->setChecked(true);
    else if (index == 4)
        ui->pushButtonSettings->setChecked(true);
}

void BusinessMainWindow::setupShiftsSection()
{
    setupShiftMonthCalendar();
    setupShiftDayView();

    connect(ui->pushButtonCreateShift, &QPushButton::clicked, this, &BusinessMainWindow::onCreateShiftClicked);
    connect(ui->pushButtonEditShift, &QPushButton::clicked, this, &BusinessMainWindow::onEditShiftClicked);
    connect(ui->pushButtonDeleteShift, &QPushButton::clicked, this, &BusinessMainWindow::onDeleteShiftClicked);
    connect(ui->pushButtonShiftArchiveToggle, &QPushButton::clicked, this, &BusinessMainWindow::onToggleShiftArchiveClicked);

    connect(ui->pushButtonShiftMonthView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(0, "Календарь смен на месяц");
    });
    connect(ui->pushButtonShiftDayView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(1, "Смены на выбранный день");
    });
    connect(ui->pushButtonShiftListView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(2, "Список смен");
    });

    connect(ui->pushButtonShiftPrevious, &QPushButton::clicked, this, &BusinessMainWindow::onPreviousShiftPeriodClicked);
    connect(ui->pushButtonShiftNext, &QPushButton::clicked, this, &BusinessMainWindow::onNextShiftPeriodClicked);
    connect(ui->pushButtonShiftToday, &QPushButton::clicked, this, &BusinessMainWindow::onTodayShiftPeriodClicked);

    showShiftsSubsection(0, "Календарь смен на месяц");
}

void BusinessMainWindow::setupShiftMonthCalendar()
{
    shiftMonthTable = new QTableWidget(5, 7, this);
    shiftMonthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shiftMonthTable->setSelectionMode(QAbstractItemView::NoSelection);
    shiftMonthTable->setFocusPolicy(Qt::NoFocus);
    shiftMonthTable->setWordWrap(true);
    shiftMonthTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    shiftMonthTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    shiftMonthTable->verticalHeader()->setVisible(false);
    shiftMonthTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    shiftMonthTable->setHorizontalHeaderLabels({"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"});

    for (int row = 0; row < shiftMonthTable->rowCount(); ++row)
        shiftMonthTable->setRowHeight(row, 110);

    ui->labelShiftMonthPlaceholder->hide();
    ui->verticalLayoutShiftMonth->addWidget(shiftMonthTable);
}

void BusinessMainWindow::setupShiftDayView()
{
    auto *navigationLayout = new QHBoxLayout();
    shiftDayPreviousButton = new QPushButton("<", this);
    shiftDayNextButton = new QPushButton(">", this);
    shiftDayCounterLabel = new QLabel("Смена 0 из 0", this);
    shiftDayCounterLabel->setAlignment(Qt::AlignCenter);

    navigationLayout->addWidget(shiftDayPreviousButton);
    navigationLayout->addWidget(shiftDayCounterLabel, 1);
    navigationLayout->addWidget(shiftDayNextButton);

    auto *cardFrame = new QFrame(this);
    auto *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(12);

    auto createSectionLabel = [this](const QString& text) {
        auto *label = new QLabel(text, this);
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
        return label;
    };

    shiftDayTimeLabel = new QLabel("-", this);
    shiftDayStatusLabel = new QLabel("-", this);
    shiftDayAssignedLabel = new QLabel("-", this);
    shiftDayOpenPositionsLabel = new QLabel("-", this);
    shiftDayCommentLabel = new QLabel("-", this);

    for (QLabel *label : {shiftDayTimeLabel, shiftDayStatusLabel, shiftDayAssignedLabel,
                          shiftDayOpenPositionsLabel, shiftDayCommentLabel})
    {
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

    cardLayout->addWidget(createSectionLabel("Время"));
    cardLayout->addWidget(shiftDayTimeLabel);
    cardLayout->addWidget(createSectionLabel("Статус"));
    cardLayout->addWidget(shiftDayStatusLabel);
    cardLayout->addWidget(createSectionLabel("Назначенные сотрудники"));
    cardLayout->addWidget(shiftDayAssignedLabel);
    cardLayout->addWidget(createSectionLabel("Свободные позиции"));
    cardLayout->addWidget(shiftDayOpenPositionsLabel);
    cardLayout->addWidget(createSectionLabel("Комментарий"));
    cardLayout->addWidget(shiftDayCommentLabel);
    cardLayout->addStretch();

    ui->labelShiftDayPlaceholder->hide();
    ui->verticalLayoutShiftDay->addLayout(navigationLayout);
    ui->verticalLayoutShiftDay->addWidget(cardFrame);

    connect(shiftDayPreviousButton, &QPushButton::clicked, this, [this]() {
        if (currentDayShiftIds.isEmpty() || currentDayShiftIndex <= 0)
            return;
        --currentDayShiftIndex;
        showCurrentDayShift();
    });
    connect(shiftDayNextButton, &QPushButton::clicked, this, [this]() {
        if (currentDayShiftIds.isEmpty() || currentDayShiftIndex >= currentDayShiftIds.size() - 1)
            return;
        ++currentDayShiftIndex;
        showCurrentDayShift();
    });
}

void BusinessMainWindow::showShiftsSubsection(int index, const QString& title)
{
    ui->stackedWidgetShiftContent->setCurrentIndex(index);

    if (index != 2)
        showingShiftArchive = false;

    if (index == 0)
        ui->pushButtonShiftMonthView->setChecked(true);
    else if (index == 1)
        ui->pushButtonShiftDayView->setChecked(true);
    else if (index == 2)
        ui->pushButtonShiftListView->setChecked(true);

    ui->pushButtonShiftPrevious->setEnabled(index != 2);
    ui->pushButtonShiftNext->setEnabled(index != 2);
    ui->pushButtonShiftArchiveToggle->setVisible(index == 2);

    if (index == 0)
        loadShiftMonthCalendar();
    else if (index == 1)
        loadShiftDayView();
    else if (index == 2)
        loadShiftList();

    ui->labelShiftContentTitle->setText(index == 2
        ? (showingShiftArchive ? "Архив смен" : "Список смен")
        : title);

    updateShiftListMode();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::updateShiftPeriodLabel()
{
    const int shiftViewIndex = ui->stackedWidgetShiftContent->currentIndex();

    if (shiftViewIndex == 0)
        ui->labelShiftCurrentPeriod->setText(currentShiftDate.toString("MMMM yyyy"));
    else if (shiftViewIndex == 1)
        ui->labelShiftCurrentPeriod->setText(currentShiftDate.toString("dd MMMM yyyy"));
    else
        ui->labelShiftCurrentPeriod->setText(showingShiftArchive ? "Все смены" : "Актуальные смены");
}

void BusinessMainWindow::loadShiftMonthCalendar()
{
    if (!shiftMonthTable)
        return;

    const QDate firstDay(currentShiftDate.year(), currentShiftDate.month(), 1);
    const QDate lastDay = firstDay.addMonths(1).addDays(-1);
    const int startColumn = firstDay.dayOfWeek() - 1;
    const int totalCells = startColumn + lastDay.day();
    const int requiredRows = (totalCells + 6) / 7;

    shiftMonthTable->setRowCount(requiredRows);
    for (int row = 0; row < requiredRows; ++row)
        shiftMonthTable->setRowHeight(row, 110);
    shiftMonthTable->clearContents();

    if (currentBusinessId < 0)
        return;

    QMap<QDate, QStringList> shiftsByDate;
    QSqlQuery query = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, firstDay, lastDay);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString timeRange = QString("%1-%2")
                                      .arg(query.value("start_time").toString(),
                                           query.value("end_time").toString());

        QSet<QString> positions;
        for (const ShiftAssignedEmployeeData& assignment : DatabaseManager::instance().getShiftAssignments(shiftId))
            positions.insert(assignment.positionName);
        for (const ShiftOpenPositionData& openPosition : DatabaseManager::instance().getShiftOpenPositions(shiftId))
            positions.insert(openPosition.positionName);

        QStringList sortedPositions = positions.values();
        std::sort(sortedPositions.begin(), sortedPositions.end());
        shiftsByDate[shiftDate].append(sortedPositions.isEmpty()
            ? timeRange
            : QString("%1 | %2").arg(timeRange, sortedPositions.join(", ")));
    }

    for (int day = 1; day <= lastDay.day(); ++day)
    {
        const QDate cellDate(firstDay.year(), firstDay.month(), day);
        const int cellIndex = startColumn + day - 1;
        const int row = cellIndex / 7;
        const int column = cellIndex % 7;

        QStringList lines;
        lines << QString::number(day);
        const QStringList shiftLines = shiftsByDate.value(cellDate);
        lines.append(shiftLines);

        const QString fullText = lines.join("\n");
        QString displayText = fullText;
        bool tooltipOnly = false;

        if (shiftLines.size() > 2)
        {
            displayText = QString::number(day) + "\n" + shiftLines.first() + "\nⓘ";
            tooltipOnly = true;
        }
        else if (fullText.length() > 70)
        {
            displayText = QString::number(day) + "\nⓘ";
            tooltipOnly = true;
        }

        auto *item = new QTableWidgetItem(displayText);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        item->setFlags(Qt::ItemIsEnabled);
        item->setToolTip(tooltipOnly ? fullText : QString());
        if (cellDate == QDate::currentDate())
            item->setBackground(QColor(255, 248, 220));
        shiftMonthTable->setItem(row, column, item);
    }
}

void BusinessMainWindow::loadShiftDayView()
{
    currentDayShiftIds.clear();
    currentDayShiftIndex = 0;

    if (currentBusinessId >= 0)
    {
        QSqlQuery query = DatabaseManager::instance().getShiftsForPeriod(currentBusinessId, currentShiftDate, currentShiftDate);
        while (query.next())
            currentDayShiftIds.append(query.value("id").toInt());
    }

    showCurrentDayShift();
}

void BusinessMainWindow::showCurrentDayShift()
{
    const int totalShifts = currentDayShiftIds.size();

    shiftDayPreviousButton->setEnabled(totalShifts > 1 && currentDayShiftIndex > 0);
    shiftDayNextButton->setEnabled(totalShifts > 1 && currentDayShiftIndex < totalShifts - 1);

    if (totalShifts == 0)
    {
        shiftDayCounterLabel->setText("Смен нет");
        shiftDayTimeLabel->setText(QString("На %1 смен пока нет.").arg(currentShiftDate.toString("dd.MM.yyyy")));
        shiftDayStatusLabel->setText("-");
        shiftDayAssignedLabel->setText("-");
        shiftDayOpenPositionsLabel->setText("-");
        shiftDayCommentLabel->setText("-");
        return;
    }

    shiftDayCounterLabel->setText(QString("Смена %1 из %2").arg(currentDayShiftIndex + 1).arg(totalShifts));

    const int shiftId = currentDayShiftIds.at(currentDayShiftIndex);
    QSqlQuery query = DatabaseManager::instance().getShiftById(shiftId);
    if (!query.next())
        return;

    shiftDayTimeLabel->setText(QString("%1 - %2")
                                   .arg(query.value("start_time").toString(),
                                        query.value("end_time").toString()));
    shiftDayStatusLabel->setText(query.value("status").toString());
    shiftDayAssignedLabel->setText(DatabaseManager::instance().getShiftAssignedSummary(shiftId).join("\n"));
    if (shiftDayAssignedLabel->text().isEmpty())
        shiftDayAssignedLabel->setText("-");

    shiftDayOpenPositionsLabel->setText(DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId).join("\n"));
    if (shiftDayOpenPositionsLabel->text().isEmpty())
        shiftDayOpenPositionsLabel->setText("-");

    const QString comment = query.value("comment").toString().trimmed();
    shiftDayCommentLabel->setText(comment.isEmpty() ? "-" : comment);
}

void BusinessMainWindow::updateShiftListMode()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        ui->pushButtonShiftArchiveToggle->setVisible(false);
        return;
    }

    ui->pushButtonShiftArchiveToggle->setVisible(true);
    ui->pushButtonShiftArchiveToggle->setText(
        showingShiftArchive ? "Вернуться к списку смен" : "Архив смен");
}

void BusinessMainWindow::loadShiftList()
{
    ui->listWidgetShiftList->clear();

    if (currentBusinessId < 0)
    {
        ui->labelShiftListCount->setText(showingShiftArchive ? "Смен в архиве: 0" : "Актуальных смен: 0");
        return;
    }

    QSqlQuery query = showingShiftArchive
        ? DatabaseManager::instance().getAllShifts(currentBusinessId)
        : DatabaseManager::instance().getShiftsForList(currentBusinessId, QDate::currentDate());

    int count = 0;
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString timeRange = QString("%1 - %2")
                                      .arg(query.value("start_time").toString(),
                                           query.value("end_time").toString());

        QStringList lines;
        lines << QString("%1 | %2 | %3")
                     .arg(shiftDate.toString("dd.MM.yyyy"), timeRange, query.value("status").toString());
        lines << QString("Назначены: %1")
                     .arg(DatabaseManager::instance().getShiftAssignedSummary(shiftId).join("; "));
        lines << QString("Свободные позиции: %1")
                     .arg(DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId).join("; "));

        const QString comment = query.value("comment").toString().trimmed();
        if (!comment.isEmpty())
            lines << QString("Комментарий: %1").arg(comment);

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, shiftId);
        ui->listWidgetShiftList->addItem(item);
        ++count;
    }

    ui->labelShiftListCount->setText(
        showingShiftArchive
            ? QString("Смен в архиве: %1").arg(count)
            : QString("Актуальных смен: %1").arg(count));
}

void BusinessMainWindow::setupStaffSection()
{
    ui->comboBoxEmployeeSort->addItem("ФИО: А-Я");
    ui->comboBoxEmployeeSort->addItem("ФИО: Я-А");

    connect(ui->pushButtonStaffEmployeesView, &QPushButton::clicked, this, [this]() {
        showStaffSubsection(0, "Список сотрудников");
    });
    connect(ui->pushButtonStaffPositionsView, &QPushButton::clicked, this, [this]() {
        showStaffSubsection(1, "Управление должностями");
    });
    connect(ui->pushButtonAddEmployee, &QPushButton::clicked, this, &BusinessMainWindow::onAddEmployeeClicked);
    connect(ui->listWidgetEmployees, &QListWidget::itemDoubleClicked, this, &BusinessMainWindow::onEmployeeItemDoubleClicked);
    connect(ui->comboBoxEmployeeSort, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        loadEmployees();
    });
    connect(ui->pushButtonAddPosition, &QPushButton::clicked, this, &BusinessMainWindow::onAddPositionClicked);
    connect(ui->pushButtonEditPosition, &QPushButton::clicked, this, &BusinessMainWindow::onEditPositionClicked);
    connect(ui->pushButtonDeletePosition, &QPushButton::clicked, this, &BusinessMainWindow::onDeletePositionClicked);

    loadEmployees();
    loadPositions();
    showStaffSubsection(0, "Список сотрудников");
}

void BusinessMainWindow::loadEmployees()
{
    ui->listWidgetEmployees->clear();

    if (currentBusinessId < 0)
    {
        ui->labelEmployeeCount->setText("Сотрудников: 0");
        return;
    }

    QSqlQuery query = DatabaseManager::instance().getEmployees(currentBusinessId, ui->comboBoxEmployeeSort->currentIndex() == 0);
    int count = 0;
    while (query.next())
    {
        auto *item = new QListWidgetItem(query.value("full_name").toString());
        item->setData(Qt::UserRole, query.value("id").toInt());
        ui->listWidgetEmployees->addItem(item);
        ++count;
    }
    ui->labelEmployeeCount->setText(QString("Сотрудников: %1").arg(count));
}

void BusinessMainWindow::loadPositions()
{
    ui->listWidgetPositions->clear();

    if (currentBusinessId < 0)
    {
        ui->labelPositionCount->setText("Должностей: 0");
        return;
    }

    QSqlQuery query = DatabaseManager::instance().getPositions(currentBusinessId, true);
    int count = 0;
    while (query.next())
    {
        const int positionId = query.value("id").toInt();
        const QString name = query.value("name").toString();
        const QVariant salaryValue = query.value("salary");
        const QString salaryText = salaryValue.isNull()
            ? "-"
            : QString::number(salaryValue.toDouble(), 'f', 0);
        const QStringList coveredPositionNames = DatabaseManager::instance().getCoveredPositionNames(positionId);

        QString text = QString("%1 | Оклад: %2").arg(name, salaryText);
        if (!coveredPositionNames.isEmpty())
            text += QString(" | Может заменить: %1").arg(coveredPositionNames.join(", "));

        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, positionId);
        ui->listWidgetPositions->addItem(item);
        ++count;
    }
    ui->labelPositionCount->setText(QString("Должностей: %1").arg(count));
}

void BusinessMainWindow::showStaffSubsection(int index, const QString& title)
{
    ui->stackedWidgetStaffContent->setCurrentIndex(index);
    ui->labelStaffContentTitle->setText(title);

    if (index == 0)
        ui->pushButtonStaffEmployeesView->setChecked(true);
    else if (index == 1)
        ui->pushButtonStaffPositionsView->setChecked(true);
}

void BusinessMainWindow::setupPaymentsSection()
{
    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    paymentsEmployeeListWidget = new QListWidget(this);
    paymentsEmployeeListWidget->setMinimumWidth(260);
    paymentsEmployeeListWidget->setAlternatingRowColors(true);

    auto *rightPane = new QFrame(this);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setSpacing(12);

    paymentsSummaryLabel = new QLabel("Выберите сотрудника для просмотра выплат", this);
    paymentsSummaryLabel->setWordWrap(true);

    auto *revenueLayout = new QHBoxLayout();
    auto *revenueLabel = new QLabel("Выручка:", this);
    paymentsRevenueEdit = new QLineEdit(this);
    paymentsRevenueEdit->setPlaceholderText("Введите выручку");
    auto *revenueValidator = new QDoubleValidator(0.0, 1000000000.0, 2, this);
    revenueValidator->setNotation(QDoubleValidator::StandardNotation);
    paymentsRevenueEdit->setValidator(revenueValidator);
    paymentsSaveRevenueButton = new QPushButton("Сохранить выручку", this);
    revenueLayout->addWidget(revenueLabel);
    revenueLayout->addWidget(paymentsRevenueEdit, 1);
    revenueLayout->addWidget(paymentsSaveRevenueButton);

    paymentsShiftListWidget = new QListWidget(this);
    paymentsShiftListWidget->setAlternatingRowColors(true);

    auto *buttonsLayout = new QHBoxLayout();
    paymentsMarkPaidButton = new QPushButton("Отметить смену оплаченной", this);
    paymentsMarkAllPaidButton = new QPushButton("Оплатить все", this);
    buttonsLayout->addWidget(paymentsMarkPaidButton);
    buttonsLayout->addWidget(paymentsMarkAllPaidButton);

    rightLayout->addWidget(paymentsSummaryLabel);
    rightLayout->addLayout(revenueLayout);
    rightLayout->addWidget(paymentsShiftListWidget, 1);
    rightLayout->addLayout(buttonsLayout);

    contentLayout->addWidget(paymentsEmployeeListWidget);
    contentLayout->addWidget(rightPane, 1);

    ui->labelPaymentsPlaceholder->hide();
    ui->verticalLayoutPayments->addLayout(contentLayout);

    connect(paymentsEmployeeListWidget, &QListWidget::currentRowChanged,
            this, &BusinessMainWindow::onPaymentEmployeeSelectionChanged);
    connect(paymentsShiftListWidget, &QListWidget::currentRowChanged,
            this, &BusinessMainWindow::onPaymentShiftSelectionChanged);
    connect(paymentsSaveRevenueButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onSavePaymentRevenueClicked);
    connect(paymentsMarkPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkPaymentPaidClicked);
    connect(paymentsMarkAllPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkAllPaymentsPaidClicked);

    updatePaymentRevenueEditor();
    loadPaymentsEmployees();
}

void BusinessMainWindow::loadPaymentsEmployees()
{
    paymentsEmployeeListWidget->clear();

    const QList<EmployeePaymentSummary> summaries =
        DatabaseManager::instance().getEmployeePaymentSummaries(currentBusinessId);

    for (const EmployeePaymentSummary& summary : summaries)
    {
        auto *item = new QListWidgetItem(
            QString("%1\nК выплате: %2 | Неоплачено смен: %3")
                .arg(summary.employeeName)
                .arg(QString::number(summary.totalAmount, 'f', 2))
                .arg(summary.unpaidAssignments));
        item->setData(Qt::UserRole, summary.employeeId);
        paymentsEmployeeListWidget->addItem(item);
    }

    if (paymentsEmployeeListWidget->count() > 0)
        paymentsEmployeeListWidget->setCurrentRow(0);
    else
        loadEmployeePaymentDetails(-1);
}

void BusinessMainWindow::loadEmployeePaymentDetails(int employeeId)
{
    currentPaymentItems.clear();
    paymentsShiftListWidget->clear();

    if (employeeId < 0)
    {
        paymentsSummaryLabel->setText("Данных по выплатам пока нет.");
        paymentsMarkPaidButton->setEnabled(false);
        paymentsMarkAllPaidButton->setEnabled(false);
        updatePaymentRevenueEditor();
        return;
    }

    currentPaymentItems = DatabaseManager::instance().getEmployeeShiftPayments(currentBusinessId, employeeId);

    double totalUnpaid = 0.0;
    int unpaidCount = 0;
    for (const ShiftPaymentInfo& payment : currentPaymentItems)
    {
        const double amount = calculatePaymentAmount(payment);
        QStringList lines;
        lines << QString("%1 | %2 | %3").arg(payment.shiftDate, payment.timeRange, payment.positionName);
        if (!payment.percentRate.trimmed().isEmpty())
            lines << QString("Выручка: %1").arg(payment.revenueAmount.trimmed().isEmpty() ? "-" : payment.revenueAmount);
        lines << QString("Тип оплаты: %1").arg(payment.paymentType);
        lines << QString("Сумма: %1").arg(QString::number(amount, 'f', 2));
        lines << QString("Статус: %1").arg(payment.isPaid ? "Оплачено" : "Не оплачено");

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, payment.assignmentId);
        paymentsShiftListWidget->addItem(item);

        if (!payment.isPaid)
        {
            totalUnpaid += amount;
            ++unpaidCount;
        }
    }

    const QString employeeName = paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->text().section('\n', 0, 0)
        : "Сотрудник";
    paymentsSummaryLabel->setText(
        QString("%1\nК выплате: %2\nНеоплаченных смен: %3")
            .arg(employeeName)
            .arg(QString::number(totalUnpaid, 'f', 2))
            .arg(unpaidCount));

    paymentsMarkPaidButton->setEnabled(paymentsShiftListWidget->count() > 0);
    paymentsMarkAllPaidButton->setEnabled(unpaidCount > 0);
    if (paymentsShiftListWidget->count() > 0)
        paymentsShiftListWidget->setCurrentRow(0);
    updatePaymentRevenueEditor();
}

void BusinessMainWindow::onPreviousShiftPeriodClicked()
{
    const int view = ui->stackedWidgetShiftContent->currentIndex();
    if (view == 2)
        return;

    currentShiftDate = (view == 0) ? currentShiftDate.addMonths(-1) : currentShiftDate.addDays(-1);
    if (view == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onNextShiftPeriodClicked()
{
    const int view = ui->stackedWidgetShiftContent->currentIndex();
    if (view == 2)
        return;

    currentShiftDate = (view == 0) ? currentShiftDate.addMonths(1) : currentShiftDate.addDays(1);
    if (view == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onTodayShiftPeriodClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() == 2)
    {
        showingShiftArchive = false;
        loadShiftList();
        updateShiftListMode();
        updateShiftPeriodLabel();
        ui->labelShiftContentTitle->setText("Список смен");
        return;
    }

    currentShiftDate = QDate::currentDate();
    if (ui->stackedWidgetShiftContent->currentIndex() == 0)
        loadShiftMonthCalendar();
    else
        loadShiftDayView();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onCreateShiftClicked()
{
    AddShiftDialog dialog(currentBusinessId, -1, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadShiftMonthCalendar();
        loadShiftDayView();
        loadShiftList();
        loadPaymentsEmployees();
    }
}

void BusinessMainWindow::onEditShiftClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        QMessageBox::information(this, "Смены", "Редактирование доступно в режиме списка.");
        return;
    }

    QListWidgetItem *item = ui->listWidgetShiftList->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Смены", "Сначала выберите смену для редактирования.");
        return;
    }

    AddShiftDialog dialog(currentBusinessId, item->data(Qt::UserRole).toInt(), this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadShiftMonthCalendar();
        loadShiftDayView();
        loadShiftList();
        loadPaymentsEmployees();
    }
}

void BusinessMainWindow::onDeleteShiftClicked()
{
    if (ui->stackedWidgetShiftContent->currentIndex() != 2)
    {
        QMessageBox::information(this, "Смены", "Удаление доступно в режиме списка.");
        return;
    }

    QListWidgetItem *item = ui->listWidgetShiftList->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Смены", "Сначала выберите смену для удаления.");
        return;
    }

    if (QMessageBox::question(this, "Удаление смены", "Удалить выбранную смену?") != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deleteShift(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить смену.");
        return;
    }

    loadShiftMonthCalendar();
    loadShiftDayView();
    loadShiftList();
    loadPaymentsEmployees();
}

void BusinessMainWindow::onToggleShiftArchiveClicked()
{
    showingShiftArchive = !showingShiftArchive;
    ui->labelShiftContentTitle->setText(showingShiftArchive ? "Архив смен" : "Список смен");
    loadShiftList();
    updateShiftListMode();
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onAddEmployeeClicked()
{
    AddEmployeeDialog dialog(currentBusinessId, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadEmployees();
        loadPaymentsEmployees();
    }
}

void BusinessMainWindow::onEmployeeItemDoubleClicked(QListWidgetItem *item)
{
    auto *employeeCardWindow = new EmployeeCardWindow(item->data(Qt::UserRole).toInt(), this);
    employeeCardWindow->show();
}

void BusinessMainWindow::onAddPositionClicked()
{
    PositionEditDialog dialog(currentBusinessId, -1, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.positionName().isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните наименование должности.");
        return;
    }

    if (!DatabaseManager::instance().createPosition(
            currentBusinessId,
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить должность.");
        return;
    }

    loadPositions();
}

void BusinessMainWindow::onEditPositionClicked()
{
    QListWidgetItem *item = ui->listWidgetPositions->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Должности", "Сначала выберите должность для редактирования.");
        return;
    }

    PositionEditDialog dialog(currentBusinessId, item->data(Qt::UserRole).toInt(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.positionName().isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните наименование должности.");
        return;
    }

    if (!DatabaseManager::instance().updatePosition(
            item->data(Qt::UserRole).toInt(),
            dialog.positionName(),
            dialog.salaryText(),
            dialog.coveredPositionIds()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось изменить должность.");
        return;
    }

    loadPositions();
}

void BusinessMainWindow::onDeletePositionClicked()
{
    QListWidgetItem *item = ui->listWidgetPositions->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Должности", "Сначала выберите должность для удаления.");
        return;
    }

    if (QMessageBox::question(
            this,
            "Удаление должности",
            QString("Удалить должность \"%1\"?").arg(item->text())) != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deletePosition(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить должность.");
        return;
    }

    loadPositions();
}

void BusinessMainWindow::onPaymentEmployeeSelectionChanged()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    loadEmployeePaymentDetails(item ? item->data(Qt::UserRole).toInt() : -1);
}

void BusinessMainWindow::updatePaymentRevenueEditor()
{
    if (!paymentsRevenueEdit || !paymentsSaveRevenueButton || !paymentsShiftListWidget)
        return;

    const int row = paymentsShiftListWidget->currentRow();
    const bool validRow = row >= 0 && row < currentPaymentItems.size();

    if (!validRow)
    {
        paymentsRevenueEdit->clear();
        paymentsRevenueEdit->setEnabled(false);
        paymentsSaveRevenueButton->setEnabled(false);
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    const bool needsRevenue = !payment.percentRate.trimmed().isEmpty();
    paymentsRevenueEdit->setText(payment.revenueAmount);
    paymentsRevenueEdit->setEnabled(needsRevenue && !payment.isPaid);
    paymentsSaveRevenueButton->setEnabled(needsRevenue && !payment.isPaid);

    if (!needsRevenue)
        paymentsRevenueEdit->clear();
}

void BusinessMainWindow::onPaymentShiftSelectionChanged()
{
    updatePaymentRevenueEditor();
}

void BusinessMainWindow::onSavePaymentRevenueClicked()
{
    const int row = paymentsShiftListWidget ? paymentsShiftListWidget->currentRow() : -1;
    if (row < 0 || row >= currentPaymentItems.size())
    {
        QMessageBox::information(this, "Выплаты", "Сначала выберите смену.");
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    if (payment.percentRate.trimmed().isEmpty())
    {
        QMessageBox::information(this, "Выплаты", "Для этой схемы оплаты выручка не требуется.");
        return;
    }

    if (!DatabaseManager::instance().updateShiftAssignmentRevenue(payment.assignmentId, paymentsRevenueEdit->text()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить выручку.");
        return;
    }

    const int employeeId = paymentsEmployeeListWidget && paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->data(Qt::UserRole).toInt()
        : -1;

    loadPaymentsEmployees();

    if (employeeId >= 0)
    {
        for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
        {
            QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
            if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
            {
                paymentsEmployeeListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void BusinessMainWindow::onMarkPaymentPaidClicked()
{
    QListWidgetItem *item = paymentsShiftListWidget->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Выплаты", "Сначала выберите смену для отметки оплаты.");
        return;
    }

    const int row = paymentsShiftListWidget->currentRow();
    if (row >= 0 && row < currentPaymentItems.size())
    {
        const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
        if (!payment.percentRate.trimmed().isEmpty() && payment.revenueAmount.trimmed().isEmpty())
        {
            QMessageBox::information(this, "Выплаты", "Сначала укажите выручку для этой смены.");
            return;
        }
    }

    if (!DatabaseManager::instance().markShiftAssignmentPaid(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось отметить оплату смены.");
        return;
    }

    const int employeeId = paymentsEmployeeListWidget && paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->data(Qt::UserRole).toInt()
        : -1;

    loadPaymentsEmployees();

    if (employeeId >= 0)
    {
        for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
        {
            QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
            if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
            {
                paymentsEmployeeListWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void BusinessMainWindow::onMarkAllPaymentsPaidClicked()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    if (!item)
    {
        QMessageBox::information(this, "Выплаты", "Сначала выберите сотрудника.");
        return;
    }

    const int employeeId = item->data(Qt::UserRole).toInt();
    for (const ShiftPaymentInfo &payment : std::as_const(currentPaymentItems))
    {
        if (!payment.isPaid && !payment.percentRate.trimmed().isEmpty() && payment.revenueAmount.trimmed().isEmpty())
        {
            QMessageBox::information(this, "Выплаты", "Укажите выручку для всех процентных смен перед массовой оплатой.");
            return;
        }
    }

    if (!DatabaseManager::instance().markAllEmployeeAssignmentsPaid(currentBusinessId, employeeId))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось отметить все выплаты как оплаченные.");
        return;
    }

    loadPaymentsEmployees();
    for (int i = 0; i < paymentsEmployeeListWidget->count(); ++i)
    {
        QListWidgetItem *employeeItem = paymentsEmployeeListWidget->item(i);
        if (employeeItem && employeeItem->data(Qt::UserRole).toInt() == employeeId)
        {
            paymentsEmployeeListWidget->setCurrentRow(i);
            break;
        }
    }
}
