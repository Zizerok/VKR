#ifndef EMPLOYEECARDWINDOW_H
#define EMPLOYEECARDWINDOW_H

#include <QMainWindow>

class EmployeeCardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmployeeCardWindow(const QString& fullName, QWidget *parent = nullptr);
};

#endif // EMPLOYEECARDWINDOW_H
