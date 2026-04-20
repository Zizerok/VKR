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
        "is_active INTEGER DEFAULT 1,"
        "hired_date TEXT,"
        "comment TEXT,"
        "salary_rate TEXT,"
        "photo_path TEXT,"
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

    query.exec(
        "CREATE TABLE IF NOT EXISTS shifts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "business_id INTEGER NOT NULL,"
        "shift_date TEXT NOT NULL,"
        "start_time TEXT NOT NULL,"
        "end_time TEXT NOT NULL,"
        "status TEXT NOT NULL,"
        "comment TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shift_assignments ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "shift_id INTEGER NOT NULL,"
        "employee_id INTEGER NOT NULL,"
        "position_name TEXT NOT NULL,"
        "payment_type TEXT NOT NULL,"
        "hourly_rate TEXT,"
        "fixed_rate TEXT,"
        "percent_rate TEXT"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shift_open_positions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "shift_id INTEGER NOT NULL,"
        "position_name TEXT NOT NULL,"
        "employee_count INTEGER NOT NULL,"
        "payment_type TEXT NOT NULL,"
        "hourly_rate TEXT,"
        "fixed_rate TEXT,"
        "percent_rate TEXT"
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
    ensureColumn(db, "employees", "is_active", "INTEGER DEFAULT 1");
    ensureColumn(db, "employees", "hired_date", "TEXT");
    ensureColumn(db, "employees", "comment", "TEXT");
    ensureColumn(db, "employees", "salary_rate", "TEXT");
    ensureColumn(db, "employees", "photo_path", "TEXT");

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

QSqlQuery DatabaseManager::getEmployeeById(int employeeId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, business_id, full_name, last_name, first_name, middle_name, birth_date, "
        "gender, phone, vk_id, position, is_active, hired_date, comment, salary_rate, photo_path "
        "FROM employees WHERE id = ?"
        );
    query.addBindValue(employeeId);
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

QStringList DatabaseManager::getCoveredPositionNamesByPositionName(int businessId, const QString& positionName)
{
    QSqlQuery positionQuery(db);
    positionQuery.prepare("SELECT id FROM positions WHERE business_id = ? AND name = ?");
    positionQuery.addBindValue(businessId);
    positionQuery.addBindValue(positionName);

    if (!positionQuery.exec() || !positionQuery.next())
        return {};

    return getCoveredPositionNames(positionQuery.value(0).toInt());
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
        "business_id, full_name, last_name, first_name, middle_name, birth_date, gender, phone, vk_id, position, "
        "is_active, hired_date, comment, salary_rate, photo_path"
        ") VALUES ("
        ":business_id, :full_name, :last_name, :first_name, :middle_name, :birth_date, :gender, :phone, :vk_id, :position, "
        ":is_active, :hired_date, :comment, :salary_rate, :photo_path"
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
    query.bindValue(":is_active", 1);
    query.bindValue(":hired_date", QDate::currentDate().toString(Qt::ISODate));
    query.bindValue(":comment", "");
    query.bindValue(":salary_rate", "");
    query.bindValue(":photo_path", "");

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "CREATE EMPLOYEE ERROR:" << query.lastError().text();

    return ok;
}

bool DatabaseManager::updateEmployee(int employeeId,
                                     const QString& lastName,
                                     const QString& firstName,
                                     const QString& middleName,
                                     const QDate& birthDate,
                                     const QString& gender,
                                     const QString& phone,
                                     const QString& vkId,
                                     const QString& position,
                                     bool isActive,
                                     const QDate& hiredDate,
                                     const QString& comment,
                                     const QString& salaryRate)
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
        "UPDATE employees SET "
        "full_name = :full_name, "
        "last_name = :last_name, "
        "first_name = :first_name, "
        "middle_name = :middle_name, "
        "birth_date = :birth_date, "
        "gender = :gender, "
        "phone = :phone, "
        "vk_id = :vk_id, "
        "position = :position, "
        "is_active = :is_active, "
        "hired_date = :hired_date, "
        "comment = :comment, "
        "salary_rate = :salary_rate "
        "WHERE id = :id"
        );

    query.bindValue(":full_name", fullName);
    query.bindValue(":last_name", trimmedLastName);
    query.bindValue(":first_name", trimmedFirstName);
    query.bindValue(":middle_name", trimmedMiddleName);
    query.bindValue(":birth_date", birthDate.toString(Qt::ISODate));
    query.bindValue(":gender", gender);
    query.bindValue(":phone", phone.trimmed());
    query.bindValue(":vk_id", vkId.trimmed());
    query.bindValue(":position", position);
    query.bindValue(":is_active", isActive ? 1 : 0);
    query.bindValue(":hired_date", hiredDate.toString(Qt::ISODate));
    query.bindValue(":comment", comment.trimmed());
    query.bindValue(":salary_rate", salaryRate.trimmed());
    query.bindValue(":id", employeeId);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "UPDATE EMPLOYEE ERROR:" << query.lastError().text();

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

bool DatabaseManager::createShift(int businessId,
                                  const QDate& shiftDate,
                                  const QTime& startTime,
                                  const QTime& endTime,
                                  const QString& status,
                                  const QString& comment,
                                  const QList<ShiftAssignedEmployeeData>& assignedEmployees,
                                  const QList<ShiftOpenPositionData>& openPositions)
{
    if (!db.transaction())
        qDebug() << "CREATE SHIFT TRANSACTION START ERROR:" << db.lastError().text();

    QSqlQuery shiftQuery(db);
    shiftQuery.prepare(
        "INSERT INTO shifts (business_id, shift_date, start_time, end_time, status, comment) "
        "VALUES (:business_id, :shift_date, :start_time, :end_time, :status, :comment)"
        );
    shiftQuery.bindValue(":business_id", businessId);
    shiftQuery.bindValue(":shift_date", shiftDate.toString(Qt::ISODate));
    shiftQuery.bindValue(":start_time", startTime.toString("HH:mm"));
    shiftQuery.bindValue(":end_time", endTime.toString("HH:mm"));
    shiftQuery.bindValue(":status", status.trimmed());
    shiftQuery.bindValue(":comment", comment.trimmed());

    if (!shiftQuery.exec())
    {
        qDebug() << "CREATE SHIFT ERROR:" << shiftQuery.lastError().text();
        db.rollback();
        return false;
    }

    const int shiftId = shiftQuery.lastInsertId().toInt();

    for (const ShiftAssignedEmployeeData& assignment : assignedEmployees)
    {
        QSqlQuery assignmentQuery(db);
        assignmentQuery.prepare(
            "INSERT INTO shift_assignments ("
            "shift_id, employee_id, position_name, payment_type, hourly_rate, fixed_rate, percent_rate"
            ") VALUES ("
            ":shift_id, :employee_id, :position_name, :payment_type, :hourly_rate, :fixed_rate, :percent_rate"
            ")"
            );
        assignmentQuery.bindValue(":shift_id", shiftId);
        assignmentQuery.bindValue(":employee_id", assignment.employeeId);
        assignmentQuery.bindValue(":position_name", assignment.positionName.trimmed());
        assignmentQuery.bindValue(":payment_type", assignment.paymentType.trimmed());
        assignmentQuery.bindValue(":hourly_rate", assignment.hourlyRate.trimmed());
        assignmentQuery.bindValue(":fixed_rate", assignment.fixedRate.trimmed());
        assignmentQuery.bindValue(":percent_rate", assignment.percentRate.trimmed());

        if (!assignmentQuery.exec())
        {
            qDebug() << "CREATE SHIFT ASSIGNMENT ERROR:" << assignmentQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    for (const ShiftOpenPositionData& openPosition : openPositions)
    {
        QSqlQuery openPositionQuery(db);
        openPositionQuery.prepare(
            "INSERT INTO shift_open_positions ("
            "shift_id, position_name, employee_count, payment_type, hourly_rate, fixed_rate, percent_rate"
            ") VALUES ("
            ":shift_id, :position_name, :employee_count, :payment_type, :hourly_rate, :fixed_rate, :percent_rate"
            ")"
            );
        openPositionQuery.bindValue(":shift_id", shiftId);
        openPositionQuery.bindValue(":position_name", openPosition.positionName.trimmed());
        openPositionQuery.bindValue(":employee_count", openPosition.employeeCount);
        openPositionQuery.bindValue(":payment_type", openPosition.paymentType.trimmed());
        openPositionQuery.bindValue(":hourly_rate", openPosition.hourlyRate.trimmed());
        openPositionQuery.bindValue(":fixed_rate", openPosition.fixedRate.trimmed());
        openPositionQuery.bindValue(":percent_rate", openPosition.percentRate.trimmed());

        if (!openPositionQuery.exec())
        {
            qDebug() << "CREATE SHIFT OPEN POSITION ERROR:" << openPositionQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit())
    {
        qDebug() << "CREATE SHIFT TRANSACTION COMMIT ERROR:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

QSqlQuery DatabaseManager::getShiftsForList(int businessId, const QDate& fromDate)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, shift_date, start_time, end_time, status, comment "
        "FROM shifts "
        "WHERE business_id = :business_id AND shift_date >= :from_date "
        "ORDER BY shift_date ASC, start_time ASC"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":from_date", fromDate.toString(Qt::ISODate));
    query.exec();
    return query;
}

QSqlQuery DatabaseManager::getAllShifts(int businessId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, shift_date, start_time, end_time, status, comment "
        "FROM shifts "
        "WHERE business_id = :business_id "
        "ORDER BY shift_date DESC, start_time DESC"
        );
    query.bindValue(":business_id", businessId);
    query.exec();
    return query;
}

QStringList DatabaseManager::getShiftAssignedSummary(int shiftId)
{
    QStringList summary;

    QSqlQuery query(db);
    query.prepare(
        "SELECT e.full_name, sa.position_name "
        "FROM shift_assignments sa "
        "JOIN employees e ON e.id = sa.employee_id "
        "WHERE sa.shift_id = ? "
        "ORDER BY sa.position_name ASC, e.full_name ASC"
        );
    query.addBindValue(shiftId);
    query.exec();

    while (query.next())
    {
        const QString employeeName = query.value(0).toString();
        const QString positionName = query.value(1).toString();
        summary << QString("%1 — %2").arg(positionName, employeeName);
    }

    return summary;
}

QStringList DatabaseManager::getShiftOpenPositionsSummary(int shiftId)
{
    QStringList summary;

    QSqlQuery query(db);
    query.prepare(
        "SELECT position_name, employee_count "
        "FROM shift_open_positions "
        "WHERE shift_id = ? "
        "ORDER BY position_name ASC"
        );
    query.addBindValue(shiftId);
    query.exec();

    while (query.next())
    {
        summary << QString("%1 — %2")
                       .arg(query.value("position_name").toString())
                       .arg(query.value("employee_count").toInt());
    }

    return summary;
}
