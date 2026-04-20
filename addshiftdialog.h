#ifndef ADDSHIFTDIALOG_H
#define ADDSHIFTDIALOG_H

#include <QDialog>
#include <QList>

#include "databasemanager.h"

class QComboBox;
class QDateEdit;
class QListWidget;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QTimeEdit;
class QWidget;

class AddShiftDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddShiftDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId;
    QList<ShiftAssignedEmployeeData> assignedEmployees;
    QList<ShiftOpenPositionData> openPositions;

    QDateEdit *shiftDateEdit;
    QTimeEdit *startTimeEdit;
    QTimeEdit *endTimeEdit;
    QComboBox *statusComboBox;
    QTextEdit *commentEdit;

    QComboBox *assignedPositionComboBox;
    QComboBox *assignedEmployeeComboBox;
    QComboBox *assignedPaymentTypeComboBox;
    QLineEdit *assignedHourlyRateEdit;
    QLineEdit *assignedFixedRateEdit;
    QLineEdit *assignedPercentRateEdit;
    QListWidget *assignedListWidget;
    QPushButton *removeAssignedButton;

    QComboBox *openPositionComboBox;
    QSpinBox *openCountSpinBox;
    QComboBox *openPaymentTypeComboBox;
    QLineEdit *openHourlyRateEdit;
    QLineEdit *openFixedRateEdit;
    QLineEdit *openPercentRateEdit;
    QListWidget *openListWidget;
    QPushButton *removeOpenPositionButton;

    void buildUi();
    void loadEmployees();
    void loadPositions();
    void updatePaymentFields(QComboBox *paymentTypeComboBox,
                             QLineEdit *hourlyRateEdit,
                             QLineEdit *fixedRateEdit,
                             QLineEdit *percentRateEdit);
    void refreshAssignedList();
    void refreshOpenPositionsList();
    void addAssignedEmployee();
    void removeAssignedEmployee();
    void addOpenPosition();
    void removeOpenPosition();
    void saveShift();
};

#endif // ADDSHIFTDIALOG_H
