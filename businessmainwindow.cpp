#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"
#include "addemployeedialog.h"
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

void BusinessMainWindow::onAddEmployeeClicked()
{
    AddEmployeeDialog dialog(currentBusinessId, this);
    if (dialog.exec() == QDialog::Accepted)
        loadEmployees();
}

void BusinessMainWindow::onEmployeeItemDoubleClicked(QListWidgetItem *item)
{
    auto *employeeCardWindow = new EmployeeCardWindow(item->text(), this);
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
