#ifndef POSITIONMANAGEMENTDIALOG_H
#define POSITIONMANAGEMENTDIALOG_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;

class PositionManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PositionManagementDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId;
    QListWidget *positionsListWidget;

    void loadPositions();
    void addPosition();
    void editPosition();
    void deletePosition();
    QListWidgetItem *currentItem() const;
};

#endif // POSITIONMANAGEMENTDIALOG_H
