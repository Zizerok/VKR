#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"
#include "addemployeedialog.h"
#include "employeecardwindow.h"

#include <QListWidgetItem>

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    setupNavigation();
    setupStaffSection();
    showSection(0, "Смены");
}

BusinessMainWindow::BusinessMainWindow(int currentUserId,int businessId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    Q_UNUSED(currentUserId);
    currentBusinessId = businessId;

    setupNavigation();
    setupStaffSection();

    QString name = DatabaseManager::instance().getBusinessName(businessId);
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

    connect(ui->pushButtonAddEmployee, &QPushButton::clicked,
            this, &BusinessMainWindow::onAddEmployeeClicked);
    connect(ui->listWidgetEmployees, &QListWidget::itemDoubleClicked,
            this, &BusinessMainWindow::onEmployeeItemDoubleClicked);
    connect(ui->comboBoxEmployeeSort, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { loadEmployees(); });

    loadEmployees();
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
