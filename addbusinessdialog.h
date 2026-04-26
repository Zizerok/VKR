#ifndef ADDBUSINESSDIALOG_H
#define ADDBUSINESSDIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;

class AddBusinessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddBusinessDialog(QWidget *parent = nullptr);
    QString businessName() const;

private slots:
    void onCreateClicked();

private:
    QLineEdit *nameEdit = nullptr;
    QLabel *statusLabel = nullptr;
};

#endif // ADDBUSINESSDIALOG_H
