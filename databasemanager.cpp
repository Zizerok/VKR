#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager::DatabaseManager() {}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::connect()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database.db");

    if(db.open())
    {
        qDebug() << "Database connected";
        createTables();
        return true;
    }
    else
    {
        qDebug() << "Database error";
        return false;
    }
}

void DatabaseManager::createTables()
{
    QSqlQuery query;
 // пользователи
    query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "login TEXT UNIQUE,"
        "email TEXT,"
        "password_hash TEXT,"
        "salt TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );
// бизнеслист
    query.exec(
        "CREATE TABLE IF NOT EXISTS businesses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "owner_id INTEGER,"
        "name TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );
}


bool DatabaseManager::createUser(const QString& login,
                                 const QString& email,
                                 const QString& passwordHash,
                                 const QString& salt)
{
    QSqlQuery query;

    qDebug() << "TRY INSERT USER";

    query.prepare(
        "INSERT INTO users(login, email, password_hash, salt) "
        "VALUES (:login,:email, :password_hash,  :salt)"
        );

    query.bindValue(":login", login);
    query.bindValue(":password_hash", passwordHash);
    query.bindValue(":email", email);
    query.bindValue(":salt", salt);

    bool ok = query.exec();

    qDebug() << "INSERT OK:" << ok;
    qDebug() << "ERROR:" << query.lastError().text();

    return ok;
}

bool DatabaseManager::userExists(const QString& login)
{
    QSqlQuery query(db);

    query.prepare("SELECT id FROM users WHERE login = ?");
    query.addBindValue(login);

    if(query.exec() && query.next())
        return true;

    return false;
}

QSqlQuery DatabaseManager::getBusinesses(int ownerId)
{
    QSqlQuery query;

    query.prepare("SELECT id, name FROM businesses WHERE owner_id = ?");
    query.addBindValue(ownerId);
    query.exec();

    return query;
}

bool DatabaseManager::createBusiness(int ownerId, const QString& name)
{
    QSqlQuery query;

    query.prepare("INSERT INTO businesses(owner_id, name) VALUES(?, ?)");
    query.addBindValue(ownerId);
    query.addBindValue(name);

    return query.exec();
}

QString DatabaseManager::getBusinessName(int businessId)
{
    QSqlQuery query;

    query.prepare("SELECT name FROM businesses WHERE id = ?");
    query.addBindValue(businessId);

    if(query.exec() && query.next())
    {
        return query.value(0).toString();
    }

    return "";
}
QSqlDatabase DatabaseManager::database()
{
    return db;
}
