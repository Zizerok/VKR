#include "databasemanager.h"
#include "login.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qDebug() << "APP START";
    DatabaseManager::instance().connect();
    qDebug() << "APP START1";
    Login w;
    w.show();
    qDebug() << "APP START";

    return a.exec();
}
