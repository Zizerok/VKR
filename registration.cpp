#include "registration.h"
#include "ui_registration.h"
#include "databasemanager.h"
#include "security.h"
#include <QMessageBox>


Registration::Registration(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Registration)
{
    ui->setupUi(this);
}

Registration::~Registration()
{
    delete ui;
}

void Registration::on_pushButton_Registration_clicked()
{
    QString login_str = ui->lineEdit_Username->text();
    QString password = ui->lineEdit_Password->text();
    QString repeat = ui->lineEdit_Password_repeat->text();
    QString email = ui->lineEdit_Email->text();

    if(login_str.isEmpty() || password.isEmpty() || email.isEmpty())
        return;

    if(password != repeat)
        return;

    if(DatabaseManager::instance().userExists(login_str))
    {
        QMessageBox::warning(this, "Ошибка", "Логин занят");
        return;
    }

    QString salt = Security::generateSalt();
    QString hash = Security::hashPassword(password, salt);

    if(DatabaseManager::instance().createUser(login_str, email, hash, salt))
    {
        QMessageBox::information(this, "OK", "Успешно");
        Login *loginWindow = new Login();
        loginWindow->show();
        qDebug() << "Всё ок";

    }else
    {
        QMessageBox::critical(this, "Ошибка", "Регистрация не удалась");
    }

}

