#ifndef BUSINESSMAINWINDOW_H
#define BUSINESSMAINWINDOW_H

#include <QMainWindow>
#include "databasemanager.h"

namespace Ui {
class BusinessMainWindow;
}

class BusinessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BusinessMainWindow(QWidget *parent = nullptr);
    BusinessMainWindow(int currentUserId,int businessId, QWidget *parent);
    ~BusinessMainWindow();

private:
    Ui::BusinessMainWindow *ui;
};

#endif // BUSINESSMAINWINDOW_H
