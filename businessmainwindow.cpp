#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

#include "addemployeedialog.h"
#include "addshiftdialog.h"
#include "employeecardwindow.h"
#include "positioneditdialog.h"

#include <QListWidgetItem>
#include <QMessageBox>

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    setupNavigation();
    setupShiftsSection();
    setupStaffSection();
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

    const QString name = DatabaseManager::instance().getBusinessName(businessId);
    ui->labelBusinessTitle->setText(name);
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
    connect(ui->pushButtonCreateShift, &QPushButton::clicked,
            this, &BusinessMainWindow::onCreateShiftClicked);
    connect(ui->pushButtonShiftArchiveToggle, &QPushButton::clicked,
            this, &BusinessMainWindow::onToggleShiftArchiveClicked);

    connect(ui->pushButtonShiftMonthView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(0, "Календарь смен на месяц");
    });
    connect(ui->pushButtonShiftDayView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(1, "Смены на выбранный день");
    });
    connect(ui->pushButtonShiftListView, &QPushButton::clicked, this, [this]() {
        showShiftsSubsection(2, "Список смен");
    });

    connect(ui->pushButtonShiftPrevious, &QPushButton::clicked,
            this, &BusinessMainWindow::onPreviousShiftPeriodClicked);
    connect(ui->pushButtonShiftNext, &QPushButton::clicked,
            this, &BusinessMainWindow::onNextShiftPeriodClicked);
    connect(ui->pushButtonShiftToday, &QPushButton::clicked,
            this, &BusinessMainWindow::onTodayShiftPeriodClicked);

    showShiftsSubsection(0, "Календарь смен на месяц");
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

    if (index == 2)
    {
        ui->labelShiftContentTitle->setText(showingShiftArchive ? "Архив смен" : "Список смен");
        loadShiftList();
    }
    else
    {
        ui->labelShiftContentTitle->setText(title);
    }

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
    connect(ui->pushButtonAddEmployee, &QPushButton::clicked,
            this, &BusinessMainWindow::onAddEmployeeClicked);
    connect(ui->listWidgetEmployees, &QListWidget::itemDoubleClicked,
            this, &BusinessMainWindow::onEmployeeItemDoubleClicked);
    connect(ui->comboBoxEmployeeSort, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { loadEmployees(); });
    connect(ui->pushButtonAddPosition, &QPushButton::clicked,
            this, &BusinessMainWindow::onAddPositionClicked);
    connect(ui->pushButtonEditPosition, &QPushButton::clicked,
            this, &BusinessMainWindow::onEditPositionClicked);
    connect(ui->pushButtonDeletePosition, &QPushButton::clicked,
            this, &BusinessMainWindow::onDeletePositionClicked);

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

    const bool ascending = ui->comboBoxEmployeeSort->currentIndex() == 0;
    QSqlQuery query = DatabaseManager::instance().getEmployees(currentBusinessId, ascending);

    int count = 0;
    while (query.next())
    {
        const QString fullName = query.value("full_name").toString();
        const int employeeId = query.value("id").toInt();

        auto *item = new QListWidgetItem(fullName);
        item->setData(Qt::UserRole, employeeId);
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
        const QStringList coveredPositionNames =
            DatabaseManager::instance().getCoveredPositionNames(positionId);

        QString itemText = QString("%1 | Оклад: %2").arg(name, salaryText);
        if (!coveredPositionNames.isEmpty())
            itemText += QString(" | Может заменить: %1").arg(coveredPositionNames.join(", "));

        auto *item = new QListWidgetItem(itemText);
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

void BusinessMainWindow::onPreviousShiftPeriodClicked()
{
    const int shiftViewIndex = ui->stackedWidgetShiftContent->currentIndex();
    if (shiftViewIndex == 2)
        return;

    if (shiftViewIndex == 0)
        currentShiftDate = currentShiftDate.addMonths(-1);
    else
        currentShiftDate = currentShiftDate.addDays(-1);

    updateShiftPeriodLabel();
}

void BusinessMainWindow::onNextShiftPeriodClicked()
{
    const int shiftViewIndex = ui->stackedWidgetShiftContent->currentIndex();
    if (shiftViewIndex == 2)
        return;

    if (shiftViewIndex == 0)
        currentShiftDate = currentShiftDate.addMonths(1);
    else
        currentShiftDate = currentShiftDate.addDays(1);

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
    updateShiftPeriodLabel();
}

void BusinessMainWindow::onCreateShiftClicked()
{
    AddShiftDialog dialog(currentBusinessId, this);
    if (dialog.exec() == QDialog::Accepted)
        loadShiftList();
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
        loadEmployees();
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

    const int positionId = item->data(Qt::UserRole).toInt();
    PositionEditDialog dialog(currentBusinessId, positionId, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.positionName().isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните наименование должности.");
        return;
    }

    if (!DatabaseManager::instance().updatePosition(
            positionId,
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

    const auto reply = QMessageBox::question(
        this,
        "Удаление должности",
        QString("Удалить должность \"%1\"?").arg(item->text())
        );

    if (reply != QMessageBox::Yes)
        return;

    if (!DatabaseManager::instance().deletePosition(item->data(Qt::UserRole).toInt()))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить должность.");
        return;
    }

    loadPositions();
}
