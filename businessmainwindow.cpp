#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

#include "addemployeedialog.h"
#include "addshiftdialog.h"
#include "employeecardwindow.h"
#include "positioneditdialog.h"

#include <QColor>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFormLayout>
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
#include <QTabWidget>
#include <QTextEdit>
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

QString formatPaymentDate(const QString& value)
{
    if (value.trimmed().isEmpty())
        return "-";

    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (!dateTime.isValid())
        dateTime = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");

    return dateTime.isValid()
        ? dateTime.toString("dd.MM.yyyy HH:mm")
        : value;
}

bool paymentNeedsRevenue(const ShiftPaymentInfo& payment)
{
    return payment.paymentType.contains("Процент", Qt::CaseInsensitive)
        || !payment.percentRate.trimmed().isEmpty();
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

    if (paymentNeedsRevenue(payment))
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
    setupCommunicationSection();
    setupSettingsSection();
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
    setupCommunicationSection();
    setupSettingsSection();

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

    auto *filterLayout = new QHBoxLayout();
    auto *filterLabel = new QLabel("Фильтр:", this);
    paymentsStatusFilterComboBox = new QComboBox(this);
    paymentsStatusFilterComboBox->addItems({"Не оплачено", "Оплачено", "Все"});
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(paymentsStatusFilterComboBox);
    filterLayout->addStretch();

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
    rightLayout->addLayout(filterLayout);
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
    connect(paymentsStatusFilterComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &BusinessMainWindow::onPaymentFilterChanged);
    connect(paymentsSaveRevenueButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onSavePaymentRevenueClicked);
    connect(paymentsMarkPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkPaymentPaidClicked);
    connect(paymentsMarkAllPaidButton, &QPushButton::clicked,
            this, &BusinessMainWindow::onMarkAllPaymentsPaidClicked);

    updatePaymentRevenueEditor();
    loadPaymentsEmployees();
}

void BusinessMainWindow::setupCommunicationSection()
{
    ui->labelCommunicationPlaceholder->hide();

    communicationTabWidget = new QTabWidget(this);

    auto *messagesPage = new QWidget(this);
    auto *messagesLayout = new QHBoxLayout(messagesPage);
    messagesLayout->setSpacing(16);

    auto *messageFormFrame = new QFrame(messagesPage);
    auto *messageFormLayout = new QVBoxLayout(messageFormFrame);
    messageFormLayout->setSpacing(12);

    auto *messageTitle = new QLabel("Новое сообщение", messageFormFrame);
    QFont titleFont = messageTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(13);
    messageTitle->setFont(titleFont);

    auto *messageForm = new QFormLayout();
    messageTypeComboBox = new QComboBox(messageFormFrame);
    messageTypeComboBox->addItems({"Общее объявление", "Новая смена", "Изменение смены", "Отмена смены", "Выплата"});
    messageShiftComboBox = new QComboBox(messageFormFrame);
    messageRecipientTypeComboBox = new QComboBox(messageFormFrame);
    messageRecipientTypeComboBox->addItems({"Все сотрудники", "Конкретный сотрудник", "По должности"});
    messageEmployeeComboBox = new QComboBox(messageFormFrame);
    messagePositionComboBox = new QComboBox(messageFormFrame);
    messageTextEdit = new QTextEdit(messageFormFrame);
    messageTextEdit->setMinimumHeight(140);
    messageTextEdit->setPlaceholderText("Введите текст уведомления для сотрудников");

    messageForm->addRow("Тип сообщения:", messageTypeComboBox);
    messageForm->addRow("Смена:", messageShiftComboBox);
    messageForm->addRow("Получатели:", messageRecipientTypeComboBox);
    messageForm->addRow("Сотрудник:", messageEmployeeComboBox);
    messageForm->addRow("Должность:", messagePositionComboBox);
    messageForm->addRow("Текст:", messageTextEdit);

    auto *messageButtonsLayout = new QHBoxLayout();
    sendMessageButton = new QPushButton("Отправить", messageFormFrame);
    clearMessageButton = new QPushButton("Очистить", messageFormFrame);
    messageButtonsLayout->addWidget(sendMessageButton);
    messageButtonsLayout->addWidget(clearMessageButton);

    messageFormLayout->addWidget(messageTitle);
    messageFormLayout->addLayout(messageForm);
    messageFormLayout->addLayout(messageButtonsLayout);
    messageFormLayout->addStretch();

    auto *historyFrame = new QFrame(messagesPage);
    auto *historyLayout = new QVBoxLayout(historyFrame);
    historyLayout->setSpacing(12);
    auto *historyTitle = new QLabel("История уведомлений", historyFrame);
    historyTitle->setFont(titleFont);
    notificationsHistoryListWidget = new QListWidget(historyFrame);
    notificationsHistoryListWidget->setAlternatingRowColors(true);
    historyLayout->addWidget(historyTitle);
    historyLayout->addWidget(notificationsHistoryListWidget, 1);

    messagesLayout->addWidget(messageFormFrame, 1);
    messagesLayout->addWidget(historyFrame, 1);

    auto *responsesPage = new QWidget(this);
    auto *responsesLayout = new QVBoxLayout(responsesPage);
    responsesLayout->setSpacing(12);
    auto *responsesHeaderLayout = new QHBoxLayout();
    auto *responsesTitle = new QLabel("Отклики на свободные позиции", responsesPage);
    responsesTitle->setFont(titleFont);
    auto *refreshResponsesButton = new QPushButton("Обновить", responsesPage);
    responsesHeaderLayout->addWidget(responsesTitle);
    responsesHeaderLayout->addStretch();
    responsesHeaderLayout->addWidget(refreshResponsesButton);
    shiftResponsesListWidget = new QListWidget(responsesPage);
    shiftResponsesListWidget->setAlternatingRowColors(true);
    responsesLayout->addLayout(responsesHeaderLayout);
    responsesLayout->addWidget(shiftResponsesListWidget, 1);

    communicationTabWidget->addTab(messagesPage, "Сообщения");
    communicationTabWidget->addTab(responsesPage, "Отклики на смены");
    ui->verticalLayoutCommunication->addWidget(communicationTabWidget);

    connect(messageRecipientTypeComboBox, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateMessageRecipientControls(); });
    connect(messageTypeComboBox, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateMessageTypeControls(); });
    connect(messageShiftComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (messageTypeComboBox && messageTypeComboBox->currentText() == "Новая смена")
                    messageTextEdit->setPlainText(buildShiftNotificationText(messageShiftComboBox->currentData().toInt()));
            });
    connect(sendMessageButton, &QPushButton::clicked, this, &BusinessMainWindow::onSendMessageClicked);
    connect(clearMessageButton, &QPushButton::clicked, this, &BusinessMainWindow::onClearMessageClicked);
    connect(refreshResponsesButton, &QPushButton::clicked, this, &BusinessMainWindow::onRefreshShiftResponsesClicked);

    loadMessageRecipients();
    loadUnnotifiedShiftOptions();
    updateMessageTypeControls();
    updateMessageRecipientControls();
    loadNotificationsHistory();
    loadShiftResponses();
}

void BusinessMainWindow::loadMessageRecipients()
{
    if (!messageEmployeeComboBox || !messagePositionComboBox)
        return;

    messageEmployeeComboBox->clear();
    messagePositionComboBox->clear();

    if (currentBusinessId < 0)
        return;

    QSqlQuery employees = DatabaseManager::instance().getEmployees(currentBusinessId, true);
    while (employees.next())
        messageEmployeeComboBox->addItem(employees.value("full_name").toString(), employees.value("id"));

    const QStringList positions = DatabaseManager::instance().getPositionNames(currentBusinessId);
    for (const QString& position : positions)
        messagePositionComboBox->addItem(position);
}

void BusinessMainWindow::loadUnnotifiedShiftOptions()
{
    if (!messageShiftComboBox)
        return;

    messageShiftComboBox->clear();
    if (currentBusinessId < 0)
        return;

    QSqlQuery query = DatabaseManager::instance().getShiftsWithoutNewShiftNotification(currentBusinessId);
    while (query.next())
    {
        const int shiftId = query.value("id").toInt();
        const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
        const QString label = QString("%1 | %2-%3 | %4")
                                  .arg(shiftDate.toString("dd.MM.yyyy"),
                                       query.value("start_time").toString(),
                                       query.value("end_time").toString(),
                                       query.value("status").toString());
        messageShiftComboBox->addItem(label, shiftId);
    }
}

void BusinessMainWindow::updateMessageTypeControls()
{
    if (!messageTypeComboBox || !messageShiftComboBox)
        return;

    const bool isNewShiftMessage = messageTypeComboBox->currentText() == "Новая смена";
    messageShiftComboBox->setEnabled(isNewShiftMessage);
    messageShiftComboBox->setVisible(isNewShiftMessage);

    if (isNewShiftMessage)
    {
        loadUnnotifiedShiftOptions();
        messageRecipientTypeComboBox->setCurrentIndex(0);
        messageTextEdit->setPlainText(buildShiftNotificationText(messageShiftComboBox->currentData().toInt()));
    }
    else if (messageTextEdit && messageTextEdit->toPlainText().startsWith("Открыта новая смена"))
    {
        messageTextEdit->clear();
    }
}

void BusinessMainWindow::updateMessageRecipientControls()
{
    if (!messageRecipientTypeComboBox)
        return;

    const QString recipientType = messageRecipientTypeComboBox->currentText();
    if (messageEmployeeComboBox)
        messageEmployeeComboBox->setEnabled(recipientType == "Конкретный сотрудник");
    if (messagePositionComboBox)
        messagePositionComboBox->setEnabled(recipientType == "По должности");
}

QString BusinessMainWindow::buildShiftNotificationText(int shiftId) const
{
    if (shiftId <= 0)
        return "";

    QSqlQuery query = DatabaseManager::instance().getShiftById(shiftId);
    if (!query.next())
        return "";

    const QDate shiftDate = QDate::fromString(query.value("shift_date").toString(), Qt::ISODate);
    QStringList lines;
    lines << "Открыта новая смена.";
    lines << QString("Дата: %1").arg(shiftDate.toString("dd.MM.yyyy"));
    lines << QString("Время: %1-%2")
                 .arg(query.value("start_time").toString(),
                      query.value("end_time").toString());

    const QStringList assigned = DatabaseManager::instance().getShiftAssignedSummary(shiftId);
    const QStringList openPositions = DatabaseManager::instance().getShiftOpenPositionsSummary(shiftId);

    if (!assigned.isEmpty())
        lines << QString("Уже назначены: %1").arg(assigned.join("; "));
    if (!openPositions.isEmpty())
        lines << QString("Свободные позиции: %1").arg(openPositions.join("; "));

    const QString comment = query.value("comment").toString().trimmed();
    if (!comment.isEmpty())
        lines << QString("Комментарий: %1").arg(comment);

    lines << QString("Для отклика нажмите кнопку в VK-боте или отправьте: Хочу смену %1").arg(shiftId);
    return lines.join("\n");
}

void BusinessMainWindow::loadNotificationsHistory()
{
    if (!notificationsHistoryListWidget)
        return;

    notificationsHistoryListWidget->clear();
    if (currentBusinessId < 0)
        return;

    const QList<NotificationInfo> notifications = DatabaseManager::instance().getNotifications(currentBusinessId);
    for (const NotificationInfo& notification : notifications)
    {
        QStringList lines;
        lines << QString("%1 | %2 | %3")
                     .arg(formatPaymentDate(notification.createdAt),
                          notification.channel,
                          notification.sendStatus);
        lines << QString("Тип: %1").arg(notification.notificationType);
        lines << QString("Получатели: %1").arg(notification.recipientLabel);
        lines << QString("Текст: %1").arg(notification.messageText);
        if (notification.shiftId > 0)
            lines << QString("Смена ID: %1").arg(notification.shiftId);

        notificationsHistoryListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}

void BusinessMainWindow::loadShiftResponses()
{
    if (!shiftResponsesListWidget)
        return;

    shiftResponsesListWidget->clear();
    if (currentBusinessId < 0)
        return;

    const QList<ShiftResponseInfo> responses = DatabaseManager::instance().getShiftResponses(currentBusinessId);
    if (responses.isEmpty())
    {
        shiftResponsesListWidget->addItem("Откликов пока нет. Позже сюда будут попадать ответы сотрудников из VK-бота.");
        return;
    }

    for (const ShiftResponseInfo& response : responses)
    {
        QStringList lines;
        lines << QString("%1 | Смена ID: %2 | %3")
                     .arg(formatPaymentDate(response.createdAt))
                     .arg(response.shiftId)
                     .arg(response.positionName);
        lines << QString("Сотрудник: %1").arg(response.employeeName.isEmpty() ? "Не найден" : response.employeeName);
        lines << QString("VK ID: %1").arg(response.vkId.isEmpty() ? "-" : response.vkId);
        lines << QString("Статус: %1").arg(response.responseStatus);
        if (!response.responseMessage.trimmed().isEmpty())
            lines << QString("Комментарий: %1").arg(response.responseMessage);

        shiftResponsesListWidget->addItem(new QListWidgetItem(lines.join("\n")));
    }
}

void BusinessMainWindow::setupSettingsSection()
{
    ui->labelSettingsPlaceholder->hide();

    auto *settingsFrame = new QFrame(this);
    auto *settingsLayout = new QVBoxLayout(settingsFrame);
    settingsLayout->setSpacing(14);

    auto *titleLabel = new QLabel("Настройки VK-интеграции", settingsFrame);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(13);
    titleLabel->setFont(titleFont);

    auto *descriptionLabel = new QLabel(
        "Эти параметры будут использоваться для связи desktop-приложения с VK-ботом сообщества и backend-сервером.",
        settingsFrame);
    descriptionLabel->setWordWrap(true);

    auto *formLayout = new QFormLayout();
    vkGroupIdEdit = new QLineEdit(settingsFrame);
    vkGroupIdEdit->setPlaceholderText("Например: 123456789");
    vkCommunityTokenEdit = new QLineEdit(settingsFrame);
    vkCommunityTokenEdit->setPlaceholderText("Токен сообщества VK");
    vkCommunityTokenEdit->setEchoMode(QLineEdit::Password);
    vkBackendUrlEdit = new QLineEdit(settingsFrame);
    vkBackendUrlEdit->setPlaceholderText("https://example.ru/vk/callback");
    vkEnabledCheckBox = new QCheckBox("Интеграция включена", settingsFrame);

    formLayout->addRow("ID группы VK:", vkGroupIdEdit);
    formLayout->addRow("Токен сообщества:", vkCommunityTokenEdit);
    formLayout->addRow("URL backend-сервера:", vkBackendUrlEdit);
    formLayout->addRow("", vkEnabledCheckBox);

    auto *buttonsLayout = new QHBoxLayout();
    auto *saveSettingsButton = new QPushButton("Сохранить настройки", settingsFrame);
    auto *checkConnectionButton = new QPushButton("Проверить подключение", settingsFrame);
    buttonsLayout->addWidget(saveSettingsButton);
    buttonsLayout->addWidget(checkConnectionButton);
    buttonsLayout->addStretch();

    vkConnectionStatusLabel = new QLabel("Статус: настройки не проверялись", settingsFrame);
    vkConnectionStatusLabel->setWordWrap(true);

    settingsLayout->addWidget(titleLabel);
    settingsLayout->addWidget(descriptionLabel);
    settingsLayout->addLayout(formLayout);
    settingsLayout->addLayout(buttonsLayout);
    settingsLayout->addWidget(vkConnectionStatusLabel);
    settingsLayout->addStretch();

    ui->verticalLayoutSettings->addWidget(settingsFrame);

    connect(saveSettingsButton, &QPushButton::clicked, this, &BusinessMainWindow::onSaveVkSettingsClicked);
    connect(checkConnectionButton, &QPushButton::clicked, this, &BusinessMainWindow::onCheckVkConnectionClicked);

    loadVkSettings();
}

void BusinessMainWindow::loadVkSettings()
{
    if (currentBusinessId < 0 || !vkGroupIdEdit || !vkCommunityTokenEdit || !vkBackendUrlEdit || !vkEnabledCheckBox)
        return;

    const VkSettingsData settings = DatabaseManager::instance().getVkSettings(currentBusinessId);
    vkGroupIdEdit->setText(settings.groupId);
    vkCommunityTokenEdit->setText(settings.communityToken);
    vkBackendUrlEdit->setText(settings.backendUrl);
    vkEnabledCheckBox->setChecked(settings.isEnabled);
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
    int visibleCount = 0;
    for (int i = 0; i < currentPaymentItems.size(); ++i)
    {
        const ShiftPaymentInfo& payment = currentPaymentItems.at(i);
        const double amount = calculatePaymentAmount(payment);
        if (!payment.isPaid)
        {
            totalUnpaid += amount;
            ++unpaidCount;
        }

        if (!paymentMatchesCurrentFilter(payment))
            continue;

        QStringList lines;
        lines << QString("%1 | %2 | %3").arg(payment.shiftDate, payment.timeRange, payment.positionName);
        if (paymentNeedsRevenue(payment))
            lines << QString("Выручка: %1").arg(payment.revenueAmount.trimmed().isEmpty() ? "-" : payment.revenueAmount);
        lines << QString("Тип оплаты: %1").arg(payment.paymentType);
        lines << QString("Сумма: %1").arg(QString::number(amount, 'f', 2));
        lines << QString("Статус: %1").arg(payment.isPaid ? "Оплачено" : "Не оплачено");

        if (payment.isPaid)
            lines << QString("Дата оплаты: %1").arg(formatPaymentDate(payment.paidAt));

        auto *item = new QListWidgetItem(lines.join("\n"));
        item->setData(Qt::UserRole, payment.assignmentId);
        item->setData(Qt::UserRole + 1, i);
        paymentsShiftListWidget->addItem(item);
        ++visibleCount;

    }

    const QString employeeName = paymentsEmployeeListWidget->currentItem()
        ? paymentsEmployeeListWidget->currentItem()->text().section('\n', 0, 0)
        : "Сотрудник";
    paymentsSummaryLabel->setText(
        QString("%1\nК выплате: %2\nНеоплаченных смен: %3")
            .arg(employeeName)
            .arg(QString::number(totalUnpaid, 'f', 2))
            .arg(QString("%1\nЗаписей в списке: %2").arg(unpaidCount).arg(visibleCount)));

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
        loadUnnotifiedShiftOptions();
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
        loadUnnotifiedShiftOptions();
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
    loadUnnotifiedShiftOptions();
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
        loadMessageRecipients();
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
    loadMessageRecipients();
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
    loadMessageRecipients();
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
    loadMessageRecipients();
}

void BusinessMainWindow::onPaymentEmployeeSelectionChanged()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    loadEmployeePaymentDetails(item ? item->data(Qt::UserRole).toInt() : -1);
}

bool BusinessMainWindow::paymentMatchesCurrentFilter(const ShiftPaymentInfo& payment) const
{
    if (!paymentsStatusFilterComboBox)
        return true;

    switch (paymentsStatusFilterComboBox->currentIndex())
    {
    case 0:
        return !payment.isPaid;
    case 1:
        return payment.isPaid;
    default:
        return true;
    }
}

void BusinessMainWindow::updatePaymentRevenueEditor()
{
    if (!paymentsRevenueEdit || !paymentsSaveRevenueButton || !paymentsShiftListWidget)
        return;

    QListWidgetItem *item = paymentsShiftListWidget->currentItem();
    const int row = item ? item->data(Qt::UserRole + 1).toInt() : -1;
    const bool validRow = row >= 0 && row < currentPaymentItems.size();

    if (!validRow)
    {
        paymentsRevenueEdit->clear();
        paymentsRevenueEdit->setEnabled(false);
        paymentsSaveRevenueButton->setEnabled(false);
        if (paymentsMarkPaidButton)
            paymentsMarkPaidButton->setEnabled(false);
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    const bool needsRevenue = paymentNeedsRevenue(payment);
    paymentsRevenueEdit->setText(payment.revenueAmount);
    paymentsRevenueEdit->setEnabled(needsRevenue);
    paymentsSaveRevenueButton->setEnabled(needsRevenue);
    if (paymentsMarkPaidButton)
        paymentsMarkPaidButton->setEnabled(!payment.isPaid);

    if (!needsRevenue)
        paymentsRevenueEdit->clear();
}

void BusinessMainWindow::onPaymentShiftSelectionChanged()
{
    updatePaymentRevenueEditor();
}

void BusinessMainWindow::onPaymentFilterChanged()
{
    QListWidgetItem *item = paymentsEmployeeListWidget->currentItem();
    loadEmployeePaymentDetails(item ? item->data(Qt::UserRole).toInt() : -1);
}

void BusinessMainWindow::onSavePaymentRevenueClicked()
{
    QListWidgetItem *selectedPaymentItem = paymentsShiftListWidget ? paymentsShiftListWidget->currentItem() : nullptr;
    const int row = selectedPaymentItem ? selectedPaymentItem->data(Qt::UserRole + 1).toInt() : -1;
    if (row < 0 || row >= currentPaymentItems.size())
    {
        QMessageBox::information(this, "Выплаты", "Сначала выберите смену.");
        return;
    }

    const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
    if (!paymentNeedsRevenue(payment))
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

    const int row = item->data(Qt::UserRole + 1).toInt();
    if (row >= 0 && row < currentPaymentItems.size())
    {
        const ShiftPaymentInfo &payment = currentPaymentItems.at(row);
        if (paymentNeedsRevenue(payment) && payment.revenueAmount.trimmed().isEmpty())
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
        if (!payment.isPaid && paymentNeedsRevenue(payment) && payment.revenueAmount.trimmed().isEmpty())
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

void BusinessMainWindow::onSendMessageClicked()
{
    if (currentBusinessId < 0)
        return;

    const bool isNewShiftMessage = messageTypeComboBox && messageTypeComboBox->currentText() == "Новая смена";
    const int shiftId = isNewShiftMessage && messageShiftComboBox
        ? messageShiftComboBox->currentData().toInt()
        : -1;

    if (isNewShiftMessage && shiftId <= 0)
    {
        QMessageBox::warning(this, "Коммуникация", "Нет смен без уведомления. Создайте новую смену или выберите другой тип сообщения.");
        return;
    }

    QString messageText = messageTextEdit ? messageTextEdit->toPlainText().trimmed() : QString();
    if (isNewShiftMessage && messageText.isEmpty())
    {
        messageText = buildShiftNotificationText(shiftId);
        messageTextEdit->setPlainText(messageText);
    }

    if (messageText.isEmpty())
    {
        QMessageBox::warning(this, "Коммуникация", "Введите текст сообщения.");
        return;
    }

    const QString recipientType = messageRecipientTypeComboBox->currentText();
    QString recipientCode = "all";
    QString recipientLabel = "Все сотрудники";

    if (recipientType == "Конкретный сотрудник")
    {
        if (messageEmployeeComboBox->currentText().trimmed().isEmpty())
        {
            QMessageBox::warning(this, "Коммуникация", "Выберите сотрудника.");
            return;
        }
        recipientCode = "employee";
        recipientLabel = messageEmployeeComboBox->currentText();
    }
    else if (recipientType == "По должности")
    {
        if (messagePositionComboBox->currentText().trimmed().isEmpty())
        {
            QMessageBox::warning(this, "Коммуникация", "Выберите должность.");
            return;
        }
        recipientCode = "position";
        recipientLabel = messagePositionComboBox->currentText();
    }

    if (!DatabaseManager::instance().createNotification(
            currentBusinessId,
            shiftId,
            recipientCode,
            recipientLabel,
            messageTypeComboBox->currentText(),
            messageText,
            "VK",
            "Ожидает VK"))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить уведомление.");
        return;
    }

    messageTextEdit->clear();
    loadUnnotifiedShiftOptions();
    loadNotificationsHistory();
}

void BusinessMainWindow::onClearMessageClicked()
{
    if (messageTextEdit)
        messageTextEdit->clear();
}

void BusinessMainWindow::onRefreshShiftResponsesClicked()
{
    loadShiftResponses();
}

void BusinessMainWindow::onSaveVkSettingsClicked()
{
    if (currentBusinessId < 0)
        return;

    VkSettingsData settings;
    settings.groupId = vkGroupIdEdit ? vkGroupIdEdit->text() : QString();
    settings.communityToken = vkCommunityTokenEdit ? vkCommunityTokenEdit->text() : QString();
    settings.backendUrl = vkBackendUrlEdit ? vkBackendUrlEdit->text() : QString();
    settings.isEnabled = vkEnabledCheckBox && vkEnabledCheckBox->isChecked();

    if (!DatabaseManager::instance().saveVkSettings(currentBusinessId, settings))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить настройки VK.");
        return;
    }

    if (vkConnectionStatusLabel)
        vkConnectionStatusLabel->setText("Статус: настройки сохранены, подключение ещё не проверено.");
}

void BusinessMainWindow::onCheckVkConnectionClicked()
{
    if (!vkConnectionStatusLabel)
        return;

    const bool hasGroupId = vkGroupIdEdit && !vkGroupIdEdit->text().trimmed().isEmpty();
    const bool hasToken = vkCommunityTokenEdit && !vkCommunityTokenEdit->text().trimmed().isEmpty();
    const bool hasBackendUrl = vkBackendUrlEdit && !vkBackendUrlEdit->text().trimmed().isEmpty();

    if (hasGroupId && hasToken && hasBackendUrl)
    {
        vkConnectionStatusLabel->setText(
            "Статус: данные заполнены. Реальная проверка подключения будет добавлена после подключения backend-сервера.");
    }
    else
    {
        vkConnectionStatusLabel->setText(
            "Статус: заполните ID группы, токен сообщества и URL backend-сервера.");
    }
}
