#ifndef BUSINESSMAINWINDOW_H
#define BUSINESSMAINWINDOW_H

#include <QDate>
#include <QList>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QString>

#include "databasemanager.h"

class QTableWidget;
class QLabel;
class QListWidget;
class QPushButton;
class QTextEdit;
class QLineEdit;

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
    QTableWidget *shiftMonthTable = nullptr;
    int currentDayShiftIndex = 0;
    QList<int> currentDayShiftIds;
    QLabel *shiftDayCounterLabel = nullptr;
    QPushButton *shiftDayPreviousButton = nullptr;
    QPushButton *shiftDayNextButton = nullptr;
    QLabel *shiftDayTimeLabel = nullptr;
    QLabel *shiftDayStatusLabel = nullptr;
    QLabel *shiftDayAssignedLabel = nullptr;
    QLabel *shiftDayOpenPositionsLabel = nullptr;
    QLabel *shiftDayCommentLabel = nullptr;
    QListWidget *paymentsEmployeeListWidget = nullptr;
    QListWidget *paymentsShiftListWidget = nullptr;
    QLabel *paymentsSummaryLabel = nullptr;
    QLineEdit *paymentsRevenueEdit = nullptr;
    QPushButton *paymentsSaveRevenueButton = nullptr;
    QPushButton *paymentsMarkPaidButton = nullptr;
    QPushButton *paymentsMarkAllPaidButton = nullptr;
    QList<ShiftPaymentInfo> currentPaymentItems;

    void setupNavigation();
    void showSection(int index, const QString& sectionTitle);

    void setupShiftsSection();
    void setupShiftMonthCalendar();
    void setupShiftDayView();
    void showShiftsSubsection(int index, const QString& title);
    void updateShiftPeriodLabel();
    void loadShiftMonthCalendar();
    void loadShiftDayView();
    void showCurrentDayShift();
    void loadShiftList();
    void updateShiftListMode();

    void setupStaffSection();
    void loadEmployees();
    void loadPositions();
    void showStaffSubsection(int index, const QString& title);

    void setupPaymentsSection();
    void loadPaymentsEmployees();
    void loadEmployeePaymentDetails(int employeeId);
    void updatePaymentRevenueEditor();

private slots:
    void onPreviousShiftPeriodClicked();
    void onNextShiftPeriodClicked();
    void onTodayShiftPeriodClicked();
    void onCreateShiftClicked();
    void onEditShiftClicked();
    void onDeleteShiftClicked();
    void onToggleShiftArchiveClicked();

    void onAddEmployeeClicked();
    void onEmployeeItemDoubleClicked(QListWidgetItem *item);
    void onAddPositionClicked();
    void onEditPositionClicked();
    void onDeletePositionClicked();
    void onPaymentEmployeeSelectionChanged();
    void onPaymentShiftSelectionChanged();
    void onSavePaymentRevenueClicked();
    void onMarkPaymentPaidClicked();
    void onMarkAllPaymentsPaidClicked();
};

#endif // BUSINESSMAINWINDOW_H
