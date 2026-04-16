#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "security.h"
#include "databasemanager.h"
#include "businesslist.h"
namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_pushButton_registration_clicked();

    void on_pushButton_login_clicked();

private:
    Ui::Login *ui;
};

#endif // LOGIN_H
