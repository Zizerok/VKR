#ifndef BUSINESSLIST_H
#define BUSINESSLIST_H

#include <QDialog>
#include "databasemanager.h"
#include <QSqlQuery>
#include <QInputDialog>
#include "businessmainwindow.h"
#include <QListWidgetItem>

namespace Ui {
class BusinessList;
}

class BusinessList : public QDialog
{
    Q_OBJECT

public:
    explicit BusinessList(QWidget *parent = nullptr);
    explicit BusinessList(int userId, QWidget *parent = nullptr);

    ~BusinessList();


private slots:
    void on_pushButton_add_clicked();

    void on_listWidget_business_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::BusinessList *ui;
    void loadBusinesses(int userid);
    int currentUserId;
};

#endif // BUSINESSLIST_H
