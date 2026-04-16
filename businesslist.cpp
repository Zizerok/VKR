#include "businesslist.h"
#include "ui_businesslist.h"

BusinessList::BusinessList(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BusinessList)
{
    ui->setupUi(this);
}

BusinessList::BusinessList(int userid, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BusinessList)
{
    ui->setupUi(this);
    currentUserId = userid;
    loadBusinesses(currentUserId);
}



void BusinessList::loadBusinesses(int userid)
{
    ui->listWidget_business->clear();

    QSqlQuery query = DatabaseManager::instance().getBusinesses(userid);

    while(query.next())
    {
        QString name = query.value("name").toString();
        int id = query.value("id").toInt();

        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, id);

        ui->listWidget_business->addItem(item);
    }
}

BusinessList::~BusinessList()
{
    delete ui;
}

void BusinessList::on_pushButton_add_clicked()
{
    QString name = QInputDialog::getText(this, "Новое предприятие", "Название:");

    if(name.isEmpty()) return;

    DatabaseManager::instance().createBusiness(currentUserId, name);

    loadBusinesses(currentUserId); // 🔥 обновляем список
}



void BusinessList::on_listWidget_business_itemDoubleClicked(QListWidgetItem *item)
{
    int businessId = item->data(Qt::UserRole).toInt();

    BusinessMainWindow *w = new BusinessMainWindow(currentUserId, businessId, nullptr);
    w->show();

    this->close();
}

