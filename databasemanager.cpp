#include "databasemanager.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace
{
void ensureColumn(QSqlDatabase& db, const QString& tableName, const QString& columnName, const QString& definition)
{
    QSqlQuery infoQuery(db);
    infoQuery.exec(QString("PRAGMA table_info(%1)").arg(tableName));

    while (infoQuery.next())
    {
        if (infoQuery.value("name").toString() == columnName)
            return;
    }

    QSqlQuery alterQuery(db);
    alterQuery.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(tableName, columnName, definition));
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

    if (db.open())
    {
        qDebug() << "Database connected";
        createTables();
        return true;
    }

    qDebug() << "Database error";
    return false;
}

void DatabaseManager::createTables()
{
    QSqlQuery query(db);

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

    query.exec(
        "CREATE TABLE IF NOT EXISTS positions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "business_id INTEGER NOT NULL,"
        "name TEXT NOT NULL,"
        "salary REAL,"
        "base_position_id INTEGER,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS position_capabilities ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "position_id INTEGER NOT NULL,"
        "covered_position_id INTEGER NOT NULL"
        ")"
        );

    ensureColumn(db, "employees", "last_name", "TEXT");
    ensureColumn(db, "employees", "first_name", "TEXT");
    ensureColumn(db, "employees", "middle_name", "TEXT");
    ensureColumn(db, "employees", "birth_date", "TEXT");
    ensureColumn(db, "employees", "gender", "TEXT");
    ensureColumn(db, "employees", "phone", "TEXT");
    ensureColumn(db, "employees", "vk_id", "TEXT");
    ensureColumn(db, "employees", "position", "TEXT");

    ensureColumn(db, "positions", "salary", "REAL");
    ensureColumn(db, "positions", "base_position_id", "INTEGER");
}

bool DatabaseManager::createUser(const QString& login,
                                 const QString& email,
                                 const QString& passwordHash,
                                 const QString& salt)
{
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO users(login, email, password_hash, salt) "
        "VALUES (:login, :email, :password_hash, :salt)"
        );
    query.bindValue(":login", login);
    query.bindValue(":email", email);
    query.bindValue(":password_hash", passwordHash);
    query.bindValue(":salt", salt);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "INSERT USER ERROR:" << query.lastError().text();
    return ok;
}

bool DatabaseManager::userExists(const QString& login)
{
    QSqlQuery query(db);
    query.prepare("SELECT id FROM users WHERE login = ?");
    query.addBindValue(login);
    return query.exec() && query.next();
}

QSqlDatabase DatabaseManager::database()
{
    return db;
}

QSqlQuery DatabaseManager::getBusinesses(int ownerId)
{
    QSqlQuery query(db);
    query.prepare("SELECT id, name FROM businesses WHERE owner_id = ?");
    query.addBindValue(ownerId);
    query.exec();
    return query;
}

bool DatabaseManager::createBusiness(int ownerId, const QString& name)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO businesses(owner_id, name) VALUES(?, ?)");
    query.addBindValue(ownerId);
    query.addBindValue(name);
    return query.exec();
}

QString DatabaseManager::getBusinessName(int businessId)
{
    QSqlQuery query(db);
    query.prepare("SELECT name FROM businesses WHERE id = ?");
    query.addBindValue(businessId);

    if (query.exec() && query.next())
        return query.value(0).toString();

    return "";
}

QSqlQuery DatabaseManager::getEmployees(int businessId, bool ascending)
{
    QSqlQuery query(db);
    const QString orderBy = ascending ? "ASC" : "DESC";
    query.prepare(
        "SELECT id, full_name FROM employees "
        "WHERE business_id = ? "
        "ORDER BY full_name " + orderBy
        );
    query.addBindValue(businessId);
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getPositions(int businessId, bool ascending)
{
    QSqlQuery query(db);
    const QString orderBy = ascending ? "ASC" : "DESC";
    query.prepare(
        "SELECT id, name, salary "
        "FROM positions "
        "WHERE business_id = ? "
        "ORDER BY name " + orderBy
        );
    query.addBindValue(businessId);
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getPositionById(int positionId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, business_id, name, salary "
        "FROM positions "
        "WHERE id = ?"
        );
    query.addBindValue(positionId);
    query.exec();
    return query;
}

QList<int> DatabaseManager::getCoveredPositionIds(int positionId)
{
    QList<int> ids;
    QSqlQuery query(db);
    query.prepare(
        "SELECT covered_position_id "
        "FROM position_capabilities "
        "WHERE position_id = ?"
        );
    query.addBindValue(positionId);
    query.exec();

    while (query.next())
        ids.append(query.value(0).toInt());

    return ids;
}

QStringList DatabaseManager::getCoveredPositionNames(int positionId)
{
    QStringList names;
    QSqlQuery query(db);
    query.prepare(
        "SELECT p.name "
        "FROM position_capabilities pc "
        "JOIN positions p ON p.id = pc.covered_position_id "
        "WHERE pc.position_id = ? "
        "ORDER BY p.name ASC"
        );
    query.addBindValue(positionId);
    query.exec();

    while (query.next())
        names << query.value(0).toString();

    return names;
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

bool DatabaseManager::createPosition(int businessId,
                                     const QString& name,
                                     const QString& salaryText,
                                     const QList<int>& coveredPositionIds)
{
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO positions (business_id, name, salary) "
        "VALUES (:business_id, :name, :salary)"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":name", name.trimmed());

    const QString trimmedSalary = salaryText.trimmed();
    if (trimmedSalary.isEmpty() || trimmedSalary == "-")
        query.bindValue(":salary", QVariant(QVariant::Double));
    else
        query.bindValue(":salary", trimmedSalary.toDouble());

    if (!query.exec())
    {
        qDebug() << "CREATE POSITION ERROR:" << query.lastError().text();
        return false;
    }

    const int positionId = query.lastInsertId().toInt();
    return updatePosition(positionId, name, salaryText, coveredPositionIds);
}

bool DatabaseManager::updatePosition(int positionId,
                                     const QString& name,
                                     const QString& salaryText,
                                     const QList<int>& coveredPositionIds)
{
    QSqlQuery updateQuery(db);
    updateQuery.prepare(
        "UPDATE positions SET "
        "name = :name, salary = :salary "
        "WHERE id = :id"
        );
    updateQuery.bindValue(":name", name.trimmed());

    const QString trimmedSalary = salaryText.trimmed();
    if (trimmedSalary.isEmpty() || trimmedSalary == "-")
        updateQuery.bindValue(":salary", QVariant(QVariant::Double));
    else
        updateQuery.bindValue(":salary", trimmedSalary.toDouble());

    updateQuery.bindValue(":id", positionId);

    if (!updateQuery.exec())
    {
        qDebug() << "UPDATE POSITION ERROR:" << updateQuery.lastError().text();
        return false;
    }

    QSqlQuery deleteCapabilities(db);
    deleteCapabilities.prepare("DELETE FROM position_capabilities WHERE position_id = ?");
    deleteCapabilities.addBindValue(positionId);
    if (!deleteCapabilities.exec())
    {
        qDebug() << "DELETE POSITION CAPABILITIES ERROR:" << deleteCapabilities.lastError().text();
        return false;
    }

    for (int coveredPositionId : coveredPositionIds)
    {
        if (coveredPositionId <= 0 || coveredPositionId == positionId)
            continue;

        QSqlQuery insertCapability(db);
        insertCapability.prepare(
            "INSERT INTO position_capabilities (position_id, covered_position_id) "
            "VALUES (?, ?)"
            );
        insertCapability.addBindValue(positionId);
        insertCapability.addBindValue(coveredPositionId);

        if (!insertCapability.exec())
        {
            qDebug() << "INSERT POSITION CAPABILITY ERROR:" << insertCapability.lastError().text();
            return false;
        }
    }

    return true;
}

bool DatabaseManager::deletePosition(int positionId)
{
    QSqlQuery deleteCapabilities(db);
    deleteCapabilities.prepare(
        "DELETE FROM position_capabilities "
        "WHERE position_id = ? OR covered_position_id = ?"
        );
    deleteCapabilities.addBindValue(positionId);
    deleteCapabilities.addBindValue(positionId);
    deleteCapabilities.exec();

    QSqlQuery query(db);
    query.prepare("DELETE FROM positions WHERE id = ?");
    query.addBindValue(positionId);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "DELETE POSITION ERROR:" << query.lastError().text();

    return ok;
}

QStringList DatabaseManager::getPositionNames(int businessId)
{
    QStringList positions;
    QSqlQuery query = getPositions(businessId, true);

    while (query.next())
        positions << query.value("name").toString();

    return positions;
}
