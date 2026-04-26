#include "registration.h"

#include "databasemanager.h"
#include "security.h"
#include "ui_registration.h"

#include <QIcon>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSize>

Registration::Registration(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Registration)
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
        QFrame#frameRegistrationCard {
            background: #FFFFFF;
            border: 1px solid #ECECF2;
            border-radius: 22px;
        }
        QLabel#labelTitle {
            color: #1C1D21;
        }
        QLabel#labelSubtitle,
        QLabel#labelLoginCaption,
        QLabel#labelEmailCaption,
        QLabel#labelPasswordCaption,
        QLabel#labelPasswordRepeatCaption {
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
        QPushButton#pushButton_togglePassword,
        QPushButton#pushButton_togglePasswordRepeat {
            background: #F5F5FA;
            border: 1px solid #ECECF2;
            border-radius: 12px;
            padding: 0px;
        }
        QPushButton#pushButton_togglePassword:hover,
        QPushButton#pushButton_togglePasswordRepeat:hover {
            background: #EEF2FF;
        }
        QPushButton#pushButton_Registration {
            background: #5E81F4;
            color: #FFFFFF;
            border: none;
            border-radius: 14px;
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#pushButton_Registration:hover {
            background: #4C6FE0;
        }
        QPushButton#pushButton_Registration:pressed {
            background: #405EC8;
        }
        QPushButton#pushButton_backToLogin {
            background: #E9EDFB;
            color: #5E81F4;
            border: none;
            border-radius: 14px;
            font-size: 16px;
            font-weight: 600;
        }
        QPushButton#pushButton_backToLogin:hover {
            background: #DEE5FB;
        }
        QPushButton#pushButton_backToLogin:pressed {
            background: #D2DBFA;
        }
    )");

    const QIcon closedLockIcon("C:/Users/Dmitrii/Documents/VKR_2/assets/free-icon-padlock-3934062.png");
    const QIcon openLockIcon("C:/Users/Dmitrii/Documents/VKR_2/assets/free-icon-open-padlock-597356.png");
    ui->pushButton_togglePassword->setIcon(closedLockIcon);
    ui->pushButton_togglePasswordRepeat->setIcon(closedLockIcon);
    ui->pushButton_togglePassword->setIconSize(QSize(18, 18));
    ui->pushButton_togglePasswordRepeat->setIconSize(QSize(18, 18));

    auto clearStatus = [this]() {
        ui->labelStatus->clear();
    };

    connect(ui->lineEdit_Username, &QLineEdit::textChanged, this, clearStatus);
    connect(ui->lineEdit_Email, &QLineEdit::textChanged, this, clearStatus);
    connect(ui->lineEdit_Password, &QLineEdit::textChanged, this, clearStatus);
    connect(ui->lineEdit_Password_repeat, &QLineEdit::textChanged, this, clearStatus);

    connect(ui->pushButton_togglePassword, &QPushButton::clicked, this, [this, closedLockIcon, openLockIcon]() {
        const bool hidden = ui->lineEdit_Password->echoMode() == QLineEdit::Password;
        ui->lineEdit_Password->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        ui->pushButton_togglePassword->setIcon(hidden ? openLockIcon : closedLockIcon);
    });
    connect(ui->pushButton_togglePasswordRepeat, &QPushButton::clicked, this, [this, closedLockIcon, openLockIcon]() {
        const bool hidden = ui->lineEdit_Password_repeat->echoMode() == QLineEdit::Password;
        ui->lineEdit_Password_repeat->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        ui->pushButton_togglePasswordRepeat->setIcon(hidden ? openLockIcon : closedLockIcon);
    });
    connect(ui->pushButton_backToLogin, &QPushButton::clicked, this, [this]() {
        auto *loginWindow = new Login();
        loginWindow->show();
        close();
    });
}

Registration::~Registration()
{
    delete ui;
}

void Registration::on_pushButton_Registration_clicked()
{
    const QString loginStr = ui->lineEdit_Username->text().trimmed();
    const QString password = ui->lineEdit_Password->text();
    const QString repeat = ui->lineEdit_Password_repeat->text();
    const QString email = ui->lineEdit_Email->text().trimmed();

    ui->labelStatus->clear();

    if (loginStr.isEmpty() || password.isEmpty() || repeat.isEmpty() || email.isEmpty())
    {
        ui->labelStatus->setText("Заполните все поля.");
        return;
    }

    const QRegularExpression emailPattern(R"(^[A-Z0-9._%+\-]+@[A-Z0-9.\-]+\.[A-Z]{2,}$)",
                                          QRegularExpression::CaseInsensitiveOption);
    if (!emailPattern.match(email).hasMatch())
    {
        ui->labelStatus->setText("Введите корректный email.");
        return;
    }

    if (password != repeat)
    {
        ui->labelStatus->setText("Пароли не совпадают.");
        return;
    }

    if (DatabaseManager::instance().userExists(loginStr))
    {
        ui->labelStatus->setText("Этот логин уже занят.");
        return;
    }

    const QString salt = Security::generateSalt();
    const QString hash = Security::hashPassword(password, salt);

    if (DatabaseManager::instance().createUser(loginStr, email, hash, salt))
    {
        auto *loginWindow = new Login();
        loginWindow->show();
        close();
    }
    else
    {
        ui->labelStatus->setText("Регистрация не удалась. Попробуйте ещё раз.");
    }
}
