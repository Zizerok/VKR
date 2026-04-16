#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDate>
#include <QStringList>

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
    QSqlQuery getEmployees(int businessId, bool ascending = true);
    QSqlQuery getPositions(int businessId, bool ascending = true);
    bool createEmployee(int businessId,
                        const QString& lastName,
                        const QString& firstName,
                        const QString& middleName,
                        const QDate& birthDate,
                        const QString& gender,
                        const QString& phone,
                        const QString& vkId,
                        const QString& position);
    bool createPosition(int businessId, const QString& name);
    bool updatePosition(int positionId, const QString& name);
    bool deletePosition(int positionId);
    QStringList getPositionNames(int businessId);

private:
    DatabaseManager();
    QSqlDatabase db;
};

#endif
