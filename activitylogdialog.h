#ifndef ACTIVITYLOGDIALOG_H
#define ACTIVITYLOGDIALOG_H

#include <QDialog>

class QComboBox;
class QListWidget;

class ActivityLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActivityLogDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId = -1;
    QComboBox *filterComboBox = nullptr;
    QListWidget *logListWidget = nullptr;

    void buildUi();
    void loadLogs();
};

#endif // ACTIVITYLOGDIALOG_H
