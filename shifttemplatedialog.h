#ifndef SHIFTTEMPLATEDIALOG_H
#define SHIFTTEMPLATEDIALOG_H

#include <QDialog>

class QButtonGroup;
class QCheckBox;
class QDateEdit;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class ShiftTemplateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShiftTemplateDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId = -1;
    QListWidget *sourceShiftListWidget = nullptr;
    QListWidget *templateListWidget = nullptr;
    QDateEdit *periodStartEdit = nullptr;
    QDateEdit *periodEndEdit = nullptr;
    QButtonGroup *repeatGroup = nullptr;
    QList<QCheckBox*> weekdayCheckBoxes;
    QLabel *templateDetailsLabel = nullptr;

    void buildUi();
    void loadSourceShifts();
    void loadTemplates();
    void updateTemplateDetails();
    QListWidgetItem *currentSourceShiftItem() const;
    QListWidgetItem *currentTemplateItem() const;
    QList<QDate> buildApplyDates() const;

private slots:
    void onSaveTemplateClicked();
    void onApplyTemplateClicked();
    void onDeleteTemplateClicked();
};

#endif // SHIFTTEMPLATEDIALOG_H
