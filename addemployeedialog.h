#ifndef ADDEMPLOYEEDIALOG_H
#define ADDEMPLOYEEDIALOG_H

#include <QDialog>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QPushButton;

class AddEmployeeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEmployeeDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId;
    QLineEdit *lastNameEdit;
    QLineEdit *firstNameEdit;
    QLineEdit *middleNameEdit;
    QDateEdit *birthDateEdit;
    QComboBox *genderComboBox;
    QLineEdit *phoneEdit;
    QLineEdit *vkIdEdit;
    QComboBox *positionComboBox;
    QPushButton *addEmployeeButton;

    void saveEmployee();
};

#endif // ADDEMPLOYEEDIALOG_H
