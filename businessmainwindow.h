#ifndef BUSINESSMAINWINDOW_H
#define BUSINESSMAINWINDOW_H

#include <QDate>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QString>

#include "databasemanager.h"

namespace Ui {
class BusinessMainWindow;
}

class BusinessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BusinessMainWindow(QWidget *parent = nullptr);
    BusinessMainWindow(int currentUserId, int businessId, QWidget *parent);
    ~BusinessMainWindow();

private:
    Ui::BusinessMainWindow *ui;
    int currentBusinessId = -1;
    QDate currentShiftDate = QDate::currentDate();
    bool showingShiftArchive = false;

    void setupNavigation();
    void showSection(int index, const QString& sectionTitle);

    void setupShiftsSection();
    void showShiftsSubsection(int index, const QString& title);
    void updateShiftPeriodLabel();
    void loadShiftList();
    void updateShiftListMode();

    void setupStaffSection();
    void loadEmployees();
    void loadPositions();
    void showStaffSubsection(int index, const QString& title);

private slots:
    void onPreviousShiftPeriodClicked();
    void onNextShiftPeriodClicked();
    void onTodayShiftPeriodClicked();
    void onCreateShiftClicked();
    void onToggleShiftArchiveClicked();

    void onAddEmployeeClicked();
    void onEmployeeItemDoubleClicked(QListWidgetItem *item);
    void onAddPositionClicked();
    void onEditPositionClicked();
    void onDeletePositionClicked();
};

#endif // BUSINESSMAINWINDOW_H
