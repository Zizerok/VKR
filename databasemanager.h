#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    bool connect();
    void createTables();
    bool createUser(const QString& login,
                    const QString& email,
                    const QString& passwordHash,
                    const QString& salt);
    bool userExists(const QString& login);

    QSqlDatabase database();
    QSqlQuery getBusinesses(int ownerId);
    bool createBusiness(int ownerId, const QString& name);
    QString getBusinessName(int businessId);

private:
    DatabaseManager();
    QSqlDatabase db;
};

#endif
