#ifndef BUSINESSMAINWINDOW_H
#define BUSINESSMAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
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
    int currentBusinessId = -1;
    void setupNavigation();
    void showSection(int index, const QString& sectionTitle);
    void setupStaffSection();
    void loadEmployees();

private slots:
    void onAddEmployeeClicked();
    void onEmployeeItemDoubleClicked(QListWidgetItem *item);
    void onManagePositionsClicked();
};

#endif // BUSINESSMAINWINDOW_H
