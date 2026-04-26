#include "login.h"

#include "registration.h"
#include "ui_login.h"

#include <QIcon>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSize>

Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);

    setModal(true);
    setFixedSize(size());
    ui->lineEdit_Username->setFocus();
    ui->labelStatus->clear();
    this->setStyleSheet(R"(
        QDialog {
            background: #F6F6F6;
        }
        QFrame#frameLoginCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLabel#labelTitle {
            color: #1C1D21;
        }
        QLabel#labelLoginCaption,
        QLabel#labelPasswordCaption {
            color: #8181A5;
            font-size: 14px;
        }
        QLabel#labelStatus {
            color: #FF808B;
            font-size: 13px;
            min-height: 18px;
        }
        QLineEdit {
            background: transparent;
            border: none;
            border-bottom: 1px solid #ECECF2;
            padding: 6px 2px 10px 2px;
            color: #1C1D21;
            font-size: 16px;
        }
        QLineEdit:focus {
            border-bottom: 2px solid #5E81F4;
        }
        QLineEdit::placeholder {
            color: #8181A5;
        }
        QPushButton#pushButton_togglePassword {
            background: #F5F5FA;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0px;
        }
        QPushButton#pushButton_togglePassword:hover {
            background: #EEF2FF;
        }
        QCheckBox {
            color: #1C1D21;
            font-size: 14px;
            spacing: 10px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 9px;
            border: 2px solid #5E81F4;
            background: #FFFFFF;
        }
        QCheckBox::indicator:checked {
            background: #5E81F4;
            border: 2px solid #5E81F4;
        }
        QPushButton#pushButton_login {
            background: #5E81F4;
            color: #FFFFFF;
            border: none;
            border-radius: 14px;
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#pushButton_login:hover {
            background: #4C6FE0;
        }
        QPushButton#pushButton_login:pressed {
            background: #405EC8;
        }
        QPushButton#pushButton_registration {
            background: #E9EDFB;
            color: #5E81F4;
            border: none;
            border-radius: 14px;
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#pushButton_registration:hover {
            background: #DEE5FB;
        }
        QPushButton#pushButton_registration:pressed {
            background: #D2DBFA;
        }
    )");

    const QIcon closedLockIcon("C:/Users/Dmitrii/Documents/VKR_2/assets/free-icon-padlock-3934062.png");
    const QIcon openLockIcon("C:/Users/Dmitrii/Documents/VKR_2/assets/free-icon-open-padlock-597356.png");
    ui->pushButton_togglePassword->setIcon(closedLockIcon);
    ui->pushButton_togglePassword->setIconSize(QSize(18, 18));

    connect(ui->lineEdit_Username, &QLineEdit::textChanged, this, [this]() {
        ui->labelStatus->clear();
    });
    connect(ui->lineEdit_password, &QLineEdit::textChanged, this, [this]() {
        ui->labelStatus->clear();
    });
    connect(ui->pushButton_togglePassword, &QPushButton::clicked, this, [this, closedLockIcon, openLockIcon]() {
        const bool hidden = ui->lineEdit_password->echoMode() == QLineEdit::Password;
        ui->lineEdit_password->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        ui->pushButton_togglePassword->setIcon(hidden ? openLockIcon : closedLockIcon);
    });
}

Login::~Login()
{
    delete ui;
}

void Login::on_pushButton_registration_clicked()
{
    auto *reg = new Registration(this);
    reg->setAttribute(Qt::WA_DeleteOnClose);
    reg->show();
    hide();
}

void Login::on_pushButton_login_clicked()
{
    const QString login = ui->lineEdit_Username->text().trimmed();
    const QString password = ui->lineEdit_password->text();

    ui->labelStatus->clear();

    if (login.isEmpty() || password.isEmpty())
    {
        ui->labelStatus->setText("Заполните логин и пароль.");
        return;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, password_hash, salt FROM users WHERE login = ?");
    query.addBindValue(login);

    if (!query.exec())
    {
        qDebug() << query.lastError().text();
        ui->labelStatus->setText("Ошибка базы данных. Проверьте подключение.");
        return;
    }

    if (query.next())
    {
        const int userId = query.value(0).toInt();
        const QString dbHash = query.value(1).toString();
        const QString salt = query.value(2).toString();
        const QString inputHash = Security::hashPassword(password, salt);

        if (inputHash == dbHash)
        {
            close();
            auto *businessList = new BusinessList(userId);
            businessList->show();
        }
        else
        {
            ui->labelStatus->setText("Неверный пароль.");
        }
    }
    else
    {
        ui->labelStatus->setText("Пользователь не найден.");
    }
}
