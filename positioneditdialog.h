#ifndef POSITIONEDITDIALOG_H
#define POSITIONEDITDIALOG_H

#include <QDialog>
#include <QList>

class QLineEdit;
class QListWidget;

class PositionEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PositionEditDialog(int businessId, int editingPositionId = -1, QWidget *parent = nullptr);

    QString positionName() const;
    QString salaryText() const;
    QList<int> coveredPositionIds() const;

private:
    int currentBusinessId;
    int currentEditingPositionId;
    QLineEdit *nameEdit;
    QLineEdit *salaryEdit;
    QListWidget *coveredPositionsList;

    void loadCoveredPositions();
    void loadCurrentPosition();
};

#endif // POSITIONEDITDIALOG_H
