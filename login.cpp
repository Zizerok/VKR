#include "login.h"
#include "registration.h"
#include "ui_login.h"
#include <qmessagebox.h>
#include <QSqlQuery>
#include <QSqlError>

Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
}

Login::~Login()
{
    delete ui;
}

void Login::on_pushButton_registration_clicked()
{
    Registration *reg = new Registration(this);
    reg->setAttribute(Qt::WA_DeleteOnClose);
    reg->show();

    this->hide();

}


void Login::on_pushButton_login_clicked()
{
    QString login = ui->lineEdit_Username->text();
    QString password = ui->lineEdit_password->text();

    if(login.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните поля");
        return;
    }

    QSqlQuery query(DatabaseManager::instance().database());

    // 1. ищем пользователя
    query.prepare("SELECT id, password_hash, salt FROM users WHERE login = ?");
    query.addBindValue(login);

    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        QMessageBox::critical(this, "Ошибка", "Ошибка базы данных");
        return;
    }

    // 2. если пользователь найден
    if(query.next())
    {
        int userId = query.value(0).toInt();
        QString dbHash = query.value(1).toString();
        QString salt   = query.value(2).toString();

        // 3. хешируем введённый пароль
        QString inputHash = Security::hashPassword(password, salt);

        // 4. сравнение
        if(inputHash == dbHash)
        {
            QMessageBox::information(this, "OK", "Успешно");
            close();
            BusinessList *business_list = new BusinessList(userId);
            business_list -> show();
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Неверный пароль");
        }
    }
    else
    {
        QMessageBox::warning(this, "Ошибка", "Пользователь не найден");
    }
}

