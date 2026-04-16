#include "businessmainwindow.h"
#include "ui_businessmainwindow.h"

BusinessMainWindow::BusinessMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
}

BusinessMainWindow::BusinessMainWindow(int currentUserId,int businessId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BusinessMainWindow)
{
    ui->setupUi(this);
    QString name = DatabaseManager::instance().getBusinessName(businessId);
    ui->textEdit->setText(name);
}


BusinessMainWindow::~BusinessMainWindow()
{
    delete ui;
}
