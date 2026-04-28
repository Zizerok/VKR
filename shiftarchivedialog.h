#ifndef SHIFTARCHIVEDIALOG_H
#define SHIFTARCHIVEDIALOG_H

#include <QDialog>

class QListWidget;

class ShiftArchiveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShiftArchiveDialog(int businessId, QWidget *parent = nullptr);

private:
    int currentBusinessId;
    QListWidget *archiveListWidget;

    void buildUi();
    void loadArchive();
};

#endif // SHIFTARCHIVEDIALOG_H
