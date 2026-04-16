#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    setupNavigation();
    showSection(0, "Смены");
}

BusinessMainWindow::BusinessMainWindow(int currentUserId,int businessId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    Q_UNUSED(currentUserId);

    setupNavigation();

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
