#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace
{
void ensureEmployeeColumn(QSqlDatabase& db, const QString& columnName, const QString& definition)
{
    QSqlQuery infoQuery(db);
    infoQuery.exec("PRAGMA table_info(employees)");

    while (infoQuery.next())
    {
        if (infoQuery.value("name").toString() == columnName)
            return;
    }

    QSqlQuery alterQuery(db);
    alterQuery.exec(QString("ALTER TABLE employees ADD COLUMN %1 %2").arg(columnName, definition));
}
}

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

    query.exec(
        "CREATE TABLE IF NOT EXISTS employees ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "business_id INTEGER NOT NULL,"
        "full_name TEXT NOT NULL,"
        "last_name TEXT,"
        "first_name TEXT,"
        "middle_name TEXT,"
        "birth_date TEXT,"
        "gender TEXT,"
        "phone TEXT,"
        "vk_id TEXT,"
        "position TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    ensureEmployeeColumn(db, "last_name", "TEXT");
    ensureEmployeeColumn(db, "first_name", "TEXT");
    ensureEmployeeColumn(db, "middle_name", "TEXT");
    ensureEmployeeColumn(db, "birth_date", "TEXT");
    ensureEmployeeColumn(db, "gender", "TEXT");
    ensureEmployeeColumn(db, "phone", "TEXT");
    ensureEmployeeColumn(db, "vk_id", "TEXT");
    ensureEmployeeColumn(db, "position", "TEXT");
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

QSqlQuery DatabaseManager::getEmployees(int businessId, bool ascending)
{
    QSqlQuery query;

    QString orderBy = ascending ? "ASC" : "DESC";
    query.prepare(
        "SELECT id, full_name FROM employees "
        "WHERE business_id = ? "
        "ORDER BY full_name " + orderBy
        );
    query.addBindValue(businessId);
    query.exec();

    return query;
}

bool DatabaseManager::createEmployee(int businessId,
                                     const QString& lastName,
                                     const QString& firstName,
                                     const QString& middleName,
                                     const QDate& birthDate,
                                     const QString& gender,
                                     const QString& phone,
                                     const QString& vkId,
                                     const QString& position)
{
    const QString trimmedLastName = lastName.trimmed();
    const QString trimmedFirstName = firstName.trimmed();
    const QString trimmedMiddleName = middleName.trimmed();

    QString fullName = trimmedLastName;
    if (!trimmedFirstName.isEmpty())
        fullName += " " + trimmedFirstName;
    if (!trimmedMiddleName.isEmpty())
        fullName += " " + trimmedMiddleName;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO employees ("
        "business_id, full_name, last_name, first_name, middle_name, birth_date, gender, phone, vk_id, position"
        ") VALUES ("
        ":business_id, :full_name, :last_name, :first_name, :middle_name, :birth_date, :gender, :phone, :vk_id, :position"
        ")"
        );

    query.bindValue(":business_id", businessId);
    query.bindValue(":full_name", fullName);
    query.bindValue(":last_name", trimmedLastName);
    query.bindValue(":first_name", trimmedFirstName);
    query.bindValue(":middle_name", trimmedMiddleName);
    query.bindValue(":birth_date", birthDate.toString(Qt::ISODate));
    query.bindValue(":gender", gender);
    query.bindValue(":phone", phone.trimmed());
    query.bindValue(":vk_id", vkId.trimmed());
    query.bindValue(":position", position);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "CREATE EMPLOYEE ERROR:" << query.lastError().text();

    return ok;
}

QSqlDatabase DatabaseManager::database()
{
    return db;
}
