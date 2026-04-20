#ifndef EMPLOYEECARDWINDOW_H
#define EMPLOYEECARDWINDOW_H

#include <QMainWindow>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;

class EmployeeCardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EmployeeCardWindow(int employeeId, QWidget *parent = nullptr);

private:
    int currentEmployeeId;
    int currentBusinessId = -1;
    bool editMode = false;

    QLabel *photoLabel;
    QLabel *headerNameLabel;
    QLabel *headerPositionLabel;
    QLabel *coveredPositionsLabel;
    QComboBox *statusComboBox;
    QPushButton *editButton;
    QPushButton *saveButton;

    QLineEdit *lastNameEdit;
    QLineEdit *firstNameEdit;
    QLineEdit *middleNameEdit;
    QDateEdit *birthDateEdit;
    QComboBox *genderComboBox;
    QLineEdit *phoneEdit;
    QLineEdit *vkIdEdit;
    QComboBox *positionComboBox;

    QDateEdit *hiredDateEdit;
    QTextEdit *commentEdit;
    QLineEdit *salaryRateEdit;

    void buildUi();
    void loadEmployee();
    void loadPositions();
    void updateHeader();
    void setEditMode(bool enabled);
    void saveEmployee();
};

#endif // EMPLOYEECARDWINDOW_H
