#include "databasemanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <algorithm>

namespace
{
struct DatabaseConfig
{
    QString host = "127.0.0.1";
    int port = 5432;
    QString databaseName = "vkr_db";
    QString userName = "postgres";
    QString password;
    QString postgresBinPath = "C:/Program Files/PostgreSQL/18/bin";
};

QString findDatabaseConfigPath()
{
    const QString fileName = "database.ini";
    const QStringList startPaths = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath(),
        "C:/Users/Dmitrii/Documents/VKR_2"
    };

    for (const QString& startPath : startPaths)
    {
        QDir dir(startPath);
        for (int i = 0; i < 6; ++i)
        {
            const QString candidate = dir.filePath(fileName);
            if (QFile::exists(candidate))
                return candidate;

            if (!dir.cdUp())
                break;
        }
    }

    return {};
}

DatabaseConfig loadDatabaseConfig()
{
    DatabaseConfig config;
    const QString configPath = findDatabaseConfigPath();

    if (configPath.isEmpty())
    {
        qDebug() << "database.ini not found. Using default PostgreSQL connection values without password.";
        return config;
    }

    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("database");
    config.host = settings.value("host", config.host).toString();
    config.port = settings.value("port", config.port).toInt();
    config.databaseName = settings.value("name", config.databaseName).toString();
    config.userName = settings.value("user", config.userName).toString();
    config.password = settings.value("password", config.password).toString();
    settings.endGroup();

    settings.beginGroup("postgres");
    config.postgresBinPath = settings.value("bin_path", config.postgresBinPath).toString();
    settings.endGroup();

    qDebug() << "Database config loaded from:" << configPath;
    return config;
}

void ensureColumn(QSqlDatabase& db, const QString& tableName, const QString& columnName, const QString& definition)
{
    QSqlQuery infoQuery(db);
    infoQuery.prepare(
        "SELECT column_name "
        "FROM information_schema.columns "
        "WHERE table_schema = 'public' "
        "AND table_name = :table_name "
        "AND column_name = :column_name"
        );
    infoQuery.bindValue(":table_name", tableName);
    infoQuery.bindValue(":column_name", columnName);

    if (infoQuery.exec() && infoQuery.next())
        return;

    QSqlQuery alterQuery(db);
    alterQuery.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(tableName, columnName, definition));
}

double parseAmount(const QString& text)
{
    bool ok = false;
    const double value = text.trimmed().replace(',', '.').toDouble(&ok);
    return ok ? value : 0.0;
}

double calculateAssignmentAmount(const ShiftPaymentInfo& payment)
{
    if (payment.paymentType == "Почасовая")
    {
        const QTime start = QTime::fromString(payment.timeRange.section(" - ", 0, 0), "HH:mm");
        const QTime end = QTime::fromString(payment.timeRange.section(" - ", 1, 1), "HH:mm");
        const double hours = start.isValid() && end.isValid()
            ? start.secsTo(end) / 3600.0
            : 0.0;
        return hours * parseAmount(payment.hourlyRate);
    }

    if (payment.paymentType == "Фиксированная ставка")
        return parseAmount(payment.fixedRate);

    if (payment.paymentType == "Ставка + процент")
        return parseAmount(payment.fixedRate)
            + parseAmount(payment.revenueAmount) * parseAmount(payment.percentRate) / 100.0;

    if (!payment.percentRate.trimmed().isEmpty())
    {
        const double percentPart = parseAmount(payment.revenueAmount) * parseAmount(payment.percentRate) / 100.0;
        if (!payment.fixedRate.trimmed().isEmpty())
            return parseAmount(payment.fixedRate) + percentPart;
        return percentPart;
    }

    return 0.0;
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
    const DatabaseConfig config = loadDatabaseConfig();

    if (QDir(config.postgresBinPath).exists())
    {
        const QByteArray currentPath = qgetenv("PATH");
        const QByteArray postgresPath = QDir::toNativeSeparators(config.postgresBinPath).toLocal8Bit();

        if (!currentPath.contains(postgresPath))
            qputenv("PATH", postgresPath + ";" + currentPath);
    }

    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(config.host);
    db.setPort(config.port);
    db.setDatabaseName(config.databaseName);
    db.setUserName(config.userName);
    db.setPassword(config.password);

    if (db.open())
    {
        qDebug() << "PostgreSQL database connected";
        createTables();
        return true;
    }

    qDebug() << "PostgreSQL database error:" << db.lastError().text();
    return false;
}

void DatabaseManager::createTables()
{
    QSqlQuery query(db);

    query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id SERIAL PRIMARY KEY,"
        "login TEXT UNIQUE,"
        "email TEXT,"
        "password_hash TEXT,"
        "salt TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS businesses ("
        "id SERIAL PRIMARY KEY,"
        "owner_id INTEGER,"
        "name TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS employees ("
        "id SERIAL PRIMARY KEY,"
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
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS positions ("
        "id SERIAL PRIMARY KEY,"
        "business_id INTEGER NOT NULL,"
        "name TEXT NOT NULL,"
        "salary REAL,"
        "base_position_id INTEGER,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS position_capabilities ("
        "id SERIAL PRIMARY KEY,"
        "position_id INTEGER NOT NULL,"
        "covered_position_id INTEGER NOT NULL"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shifts ("
        "id SERIAL PRIMARY KEY,"
        "business_id INTEGER NOT NULL,"
        "shift_date TEXT NOT NULL,"
        "start_time TEXT NOT NULL,"
        "end_time TEXT NOT NULL,"
        "status TEXT NOT NULL,"
        "comment TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shift_assignments ("
        "id SERIAL PRIMARY KEY,"
        "shift_id INTEGER NOT NULL,"
        "employee_id INTEGER NOT NULL,"
        "position_name TEXT NOT NULL,"
        "payment_type TEXT NOT NULL,"
        "hourly_rate TEXT,"
        "fixed_rate TEXT,"
        "percent_rate TEXT,"
        "revenue_amount TEXT"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shift_open_positions ("
        "id SERIAL PRIMARY KEY,"
        "shift_id INTEGER NOT NULL,"
        "position_name TEXT NOT NULL,"
        "employee_count INTEGER NOT NULL,"
        "payment_type TEXT NOT NULL,"
        "hourly_rate TEXT,"
        "fixed_rate TEXT,"
        "percent_rate TEXT"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS notifications ("
        "id SERIAL PRIMARY KEY,"
        "business_id INTEGER NOT NULL,"
        "shift_id INTEGER,"
        "recipient_type TEXT,"
        "recipient_label TEXT,"
        "notification_type TEXT,"
        "message_text TEXT,"
        "channel TEXT DEFAULT 'VK',"
        "send_status TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS shift_responses ("
        "id SERIAL PRIMARY KEY,"
        "business_id INTEGER NOT NULL,"
        "shift_id INTEGER,"
        "vk_id TEXT,"
        "employee_id INTEGER,"
        "position_name TEXT,"
        "response_status TEXT,"
        "response_message TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "processed_at TEXT"
        ")"
        );

    query.exec(
        "CREATE TABLE IF NOT EXISTS vk_settings ("
        "id SERIAL PRIMARY KEY,"
        "business_id INTEGER UNIQUE NOT NULL,"
        "group_id TEXT,"
        "community_token TEXT,"
        "backend_url TEXT,"
        "is_enabled INTEGER DEFAULT 0"
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
    ensureColumn(db, "shift_assignments", "is_paid", "INTEGER DEFAULT 0");
    ensureColumn(db, "shift_assignments", "paid_at", "TEXT");
    ensureColumn(db, "shift_assignments", "revenue_amount", "TEXT");
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
        "VALUES (:business_id, :name, :salary) "
        "RETURNING id"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":name", name.trimmed());

    const QString trimmedSalary = salaryText.trimmed();
    if (trimmedSalary.isEmpty() || trimmedSalary == "-")
        query.bindValue(":salary", QVariant(QVariant::Double));
    else
        query.bindValue(":salary", trimmedSalary.toDouble());

    if (!query.exec() || !query.next())
    {
        qDebug() << "CREATE POSITION ERROR:" << query.lastError().text();
        return false;
    }

    const int positionId = query.value("id").toInt();
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
        "VALUES (:business_id, :shift_date, :start_time, :end_time, :status, :comment) "
        "RETURNING id"
        );
    shiftQuery.bindValue(":business_id", businessId);
    shiftQuery.bindValue(":shift_date", shiftDate.toString(Qt::ISODate));
    shiftQuery.bindValue(":start_time", startTime.toString("HH:mm"));
    shiftQuery.bindValue(":end_time", endTime.toString("HH:mm"));
    shiftQuery.bindValue(":status", status.trimmed());
    shiftQuery.bindValue(":comment", comment.trimmed());

    if (!shiftQuery.exec() || !shiftQuery.next())
    {
        qDebug() << "CREATE SHIFT ERROR:" << shiftQuery.lastError().text();
        db.rollback();
        return false;
    }

    const int shiftId = shiftQuery.value("id").toInt();

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

int DatabaseManager::getLastShiftIdForBusiness(int businessId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id FROM shifts "
        "WHERE business_id = ? "
        "ORDER BY id DESC "
        "LIMIT 1"
        );
    query.addBindValue(businessId);

    if (query.exec() && query.next())
        return query.value("id").toInt();

    return -1;
}

QSqlQuery DatabaseManager::getShiftById(int shiftId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, business_id, shift_date, start_time, end_time, status, comment, created_at "
        "FROM shifts WHERE id = ?"
        );
    query.addBindValue(shiftId);
    query.exec();
    return query;
}

QList<ShiftAssignedEmployeeData> DatabaseManager::getShiftAssignments(int shiftId)
{
    QList<ShiftAssignedEmployeeData> assignments;

    QSqlQuery query(db);
    query.prepare(
        "SELECT sa.employee_id, e.full_name, sa.position_name, sa.payment_type, "
        "sa.hourly_rate, sa.fixed_rate, sa.percent_rate "
        "FROM shift_assignments sa "
        "JOIN employees e ON e.id = sa.employee_id "
        "WHERE sa.shift_id = ? "
        "ORDER BY sa.position_name ASC, e.full_name ASC"
        );
    query.addBindValue(shiftId);
    query.exec();

    while (query.next())
    {
        ShiftAssignedEmployeeData item;
        item.employeeId = query.value("employee_id").toInt();
        item.employeeName = query.value("full_name").toString();
        item.positionName = query.value("position_name").toString();
        item.paymentType = query.value("payment_type").toString();
        item.hourlyRate = query.value("hourly_rate").toString();
        item.fixedRate = query.value("fixed_rate").toString();
        item.percentRate = query.value("percent_rate").toString();
        assignments.append(item);
    }

    return assignments;
}

QList<ShiftOpenPositionData> DatabaseManager::getShiftOpenPositions(int shiftId)
{
    QList<ShiftOpenPositionData> openPositions;

    QSqlQuery query(db);
    query.prepare(
        "SELECT position_name, employee_count, payment_type, hourly_rate, fixed_rate, percent_rate "
        "FROM shift_open_positions "
        "WHERE shift_id = ? "
        "ORDER BY position_name ASC"
        );
    query.addBindValue(shiftId);
    query.exec();

    while (query.next())
    {
        ShiftOpenPositionData item;
        item.positionName = query.value("position_name").toString();
        item.employeeCount = query.value("employee_count").toInt();
        item.paymentType = query.value("payment_type").toString();
        item.hourlyRate = query.value("hourly_rate").toString();
        item.fixedRate = query.value("fixed_rate").toString();
        item.percentRate = query.value("percent_rate").toString();
        openPositions.append(item);
    }

    return openPositions;
}

bool DatabaseManager::updateShift(int shiftId,
                                  const QDate& shiftDate,
                                  const QTime& startTime,
                                  const QTime& endTime,
                                  const QString& status,
                                  const QString& comment,
                                  const QList<ShiftAssignedEmployeeData>& assignedEmployees,
                                  const QList<ShiftOpenPositionData>& openPositions)
{
    if (!db.transaction())
        qDebug() << "UPDATE SHIFT TRANSACTION START ERROR:" << db.lastError().text();

    QSqlQuery shiftQuery(db);
    shiftQuery.prepare(
        "UPDATE shifts SET "
        "shift_date = :shift_date, "
        "start_time = :start_time, "
        "end_time = :end_time, "
        "status = :status, "
        "comment = :comment "
        "WHERE id = :id"
        );
    shiftQuery.bindValue(":shift_date", shiftDate.toString(Qt::ISODate));
    shiftQuery.bindValue(":start_time", startTime.toString("HH:mm"));
    shiftQuery.bindValue(":end_time", endTime.toString("HH:mm"));
    shiftQuery.bindValue(":status", status.trimmed());
    shiftQuery.bindValue(":comment", comment.trimmed());
    shiftQuery.bindValue(":id", shiftId);

    if (!shiftQuery.exec())
    {
        qDebug() << "UPDATE SHIFT ERROR:" << shiftQuery.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery deleteAssignments(db);
    deleteAssignments.prepare("DELETE FROM shift_assignments WHERE shift_id = ?");
    deleteAssignments.addBindValue(shiftId);
    if (!deleteAssignments.exec())
    {
        qDebug() << "DELETE SHIFT ASSIGNMENTS ERROR:" << deleteAssignments.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery deleteOpenPositions(db);
    deleteOpenPositions.prepare("DELETE FROM shift_open_positions WHERE shift_id = ?");
    deleteOpenPositions.addBindValue(shiftId);
    if (!deleteOpenPositions.exec())
    {
        qDebug() << "DELETE SHIFT OPEN POSITIONS ERROR:" << deleteOpenPositions.lastError().text();
        db.rollback();
        return false;
    }

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
            qDebug() << "UPDATE SHIFT ASSIGNMENT ERROR:" << assignmentQuery.lastError().text();
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
            qDebug() << "UPDATE SHIFT OPEN POSITION ERROR:" << openPositionQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit())
    {
        qDebug() << "UPDATE SHIFT TRANSACTION COMMIT ERROR:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

bool DatabaseManager::deleteShift(int shiftId)
{
    if (!db.transaction())
        qDebug() << "DELETE SHIFT TRANSACTION START ERROR:" << db.lastError().text();

    QSqlQuery deleteAssignments(db);
    deleteAssignments.prepare("DELETE FROM shift_assignments WHERE shift_id = ?");
    deleteAssignments.addBindValue(shiftId);
    if (!deleteAssignments.exec())
    {
        qDebug() << "DELETE SHIFT ASSIGNMENTS ERROR:" << deleteAssignments.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery deleteOpenPositions(db);
    deleteOpenPositions.prepare("DELETE FROM shift_open_positions WHERE shift_id = ?");
    deleteOpenPositions.addBindValue(shiftId);
    if (!deleteOpenPositions.exec())
    {
        qDebug() << "DELETE SHIFT OPEN POSITIONS ERROR:" << deleteOpenPositions.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery deleteShiftQuery(db);
    deleteShiftQuery.prepare("DELETE FROM shifts WHERE id = ?");
    deleteShiftQuery.addBindValue(shiftId);
    if (!deleteShiftQuery.exec())
    {
        qDebug() << "DELETE SHIFT ERROR:" << deleteShiftQuery.lastError().text();
        db.rollback();
        return false;
    }

    if (!db.commit())
    {
        qDebug() << "DELETE SHIFT TRANSACTION COMMIT ERROR:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

QSqlQuery DatabaseManager::getShiftsForPeriod(int businessId, const QDate& startDate, const QDate& endDate)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, shift_date, start_time, end_time, status, comment "
        "FROM shifts "
        "WHERE business_id = :business_id "
        "AND shift_date >= :start_date "
        "AND shift_date <= :end_date "
        "ORDER BY shift_date ASC, start_time ASC"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":start_date", startDate.toString(Qt::ISODate));
    query.bindValue(":end_date", endDate.toString(Qt::ISODate));
    query.exec();
    return query;
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

QSqlQuery DatabaseManager::getShiftsWithoutNewShiftNotification(int businessId)
{
    QSqlQuery query(db);
    query.prepare(
        "SELECT s.id, s.shift_date, s.start_time, s.end_time, s.status, s.comment "
        "FROM shifts s "
        "WHERE s.business_id = :business_id "
        "AND NOT EXISTS ("
        "SELECT 1 FROM notifications n "
        "WHERE n.business_id = s.business_id "
        "AND n.shift_id = s.id "
        "AND n.notification_type = :notification_type"
        ") "
        "ORDER BY s.shift_date ASC, s.start_time ASC"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":notification_type", "Новая смена");
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

QList<EmployeePaymentSummary> DatabaseManager::getEmployeePaymentSummaries(int businessId)
{
    QList<EmployeePaymentSummary> result;

    QMap<int, EmployeePaymentSummary> summaries;
    QSqlQuery query(db);
    query.prepare(
        "SELECT sa.id, sa.employee_id, e.full_name, s.shift_date, s.start_time, s.end_time, "
        "sa.payment_type, sa.hourly_rate, sa.fixed_rate, sa.percent_rate, sa.revenue_amount, sa.is_paid, sa.paid_at "
        "FROM shift_assignments sa "
        "JOIN shifts s ON s.id = sa.shift_id "
        "JOIN employees e ON e.id = sa.employee_id "
        "WHERE s.business_id = ? "
        "ORDER BY e.full_name ASC, s.shift_date ASC, s.start_time ASC"
        );
    query.addBindValue(businessId);
    query.exec();

    while (query.next())
    {
        ShiftPaymentInfo payment;
        payment.assignmentId = query.value("id").toInt();
        payment.employeeId = query.value("employee_id").toInt();
        payment.employeeName = query.value("full_name").toString();
        payment.shiftDate = query.value("shift_date").toString();
        payment.timeRange = QString("%1 - %2")
                                .arg(query.value("start_time").toString(),
                                     query.value("end_time").toString());
        payment.paymentType = query.value("payment_type").toString();
        payment.hourlyRate = query.value("hourly_rate").toString();
        payment.fixedRate = query.value("fixed_rate").toString();
        payment.percentRate = query.value("percent_rate").toString();
        payment.revenueAmount = query.value("revenue_amount").toString();
        payment.paidAt = query.value("paid_at").toString();
        payment.isPaid = query.value("is_paid").toInt() == 1;

        EmployeePaymentSummary summary = summaries.value(payment.employeeId);
        summary.employeeId = payment.employeeId;
        summary.employeeName = payment.employeeName;

        if (!payment.isPaid)
        {
            summary.totalAmount += calculateAssignmentAmount(payment);
            ++summary.unpaidAssignments;
        }

        summaries[payment.employeeId] = summary;
    }

    result = summaries.values();
    std::sort(result.begin(), result.end(), [](const EmployeePaymentSummary& left, const EmployeePaymentSummary& right) {
        return left.employeeName.toLower() < right.employeeName.toLower();
    });
    return result;
}

QList<ShiftPaymentInfo> DatabaseManager::getEmployeeShiftPayments(int businessId, int employeeId)
{
    QList<ShiftPaymentInfo> result;

    QSqlQuery query(db);
    query.prepare(
        "SELECT sa.id, sa.shift_id, s.shift_date, s.start_time, s.end_time, sa.position_name, "
        "sa.payment_type, sa.hourly_rate, sa.fixed_rate, sa.percent_rate, sa.revenue_amount, sa.is_paid, sa.paid_at "
        "FROM shift_assignments sa "
        "JOIN shifts s ON s.id = sa.shift_id "
        "WHERE s.business_id = ? AND sa.employee_id = ? "
        "ORDER BY s.shift_date DESC, s.start_time DESC"
        );
    query.addBindValue(businessId);
    query.addBindValue(employeeId);
    query.exec();

    while (query.next())
    {
        ShiftPaymentInfo payment;
        payment.assignmentId = query.value("id").toInt();
        payment.shiftId = query.value("shift_id").toInt();
        payment.shiftDate = query.value("shift_date").toString();
        payment.timeRange = QString("%1 - %2")
                                .arg(query.value("start_time").toString(),
                                     query.value("end_time").toString());
        payment.positionName = query.value("position_name").toString();
        payment.paymentType = query.value("payment_type").toString();
        payment.hourlyRate = query.value("hourly_rate").toString();
        payment.fixedRate = query.value("fixed_rate").toString();
        payment.percentRate = query.value("percent_rate").toString();
        payment.revenueAmount = query.value("revenue_amount").toString();
        payment.paidAt = query.value("paid_at").toString();
        payment.isPaid = query.value("is_paid").toInt() == 1;
        result.append(payment);
    }

    return result;
}

bool DatabaseManager::updateShiftAssignmentRevenue(int assignmentId, const QString& revenueAmount)
{
    QSqlQuery query(db);
    query.prepare(
        "UPDATE shift_assignments SET revenue_amount = ? WHERE id = ?"
        );
    query.addBindValue(revenueAmount.trimmed());
    query.addBindValue(assignmentId);
    return query.exec();
}

bool DatabaseManager::markShiftAssignmentPaid(int assignmentId)
{
    QSqlQuery query(db);
    query.prepare(
        "UPDATE shift_assignments SET is_paid = 1, paid_at = CURRENT_TIMESTAMP WHERE id = ?"
        );
    query.addBindValue(assignmentId);
    return query.exec();
}

bool DatabaseManager::markAllEmployeeAssignmentsPaid(int businessId, int employeeId)
{
    QSqlQuery query(db);
    query.prepare(
        "UPDATE shift_assignments "
        "SET is_paid = 1, paid_at = CURRENT_TIMESTAMP "
        "WHERE employee_id = :employee_id "
        "AND is_paid = 0 "
        "AND shift_id IN (SELECT id FROM shifts WHERE business_id = :business_id)"
        );
    query.bindValue(":employee_id", employeeId);
    query.bindValue(":business_id", businessId);
    return query.exec();
}

bool DatabaseManager::createNotification(int businessId,
                                         int shiftId,
                                         const QString& recipientType,
                                         const QString& recipientLabel,
                                         const QString& notificationType,
                                         const QString& messageText,
                                         const QString& channel,
                                         const QString& sendStatus)
{
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO notifications ("
        "business_id, shift_id, recipient_type, recipient_label, notification_type, message_text, channel, send_status"
        ") VALUES ("
        ":business_id, :shift_id, :recipient_type, :recipient_label, :notification_type, :message_text, :channel, :send_status"
        ")"
        );
    query.bindValue(":business_id", businessId);
    if (shiftId > 0)
        query.bindValue(":shift_id", shiftId);
    else
        query.bindValue(":shift_id", QVariant(QVariant::Int));
    query.bindValue(":recipient_type", recipientType.trimmed());
    query.bindValue(":recipient_label", recipientLabel.trimmed());
    query.bindValue(":notification_type", notificationType.trimmed());
    query.bindValue(":message_text", messageText.trimmed());
    query.bindValue(":channel", channel.trimmed());
    query.bindValue(":send_status", sendStatus.trimmed());

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "CREATE NOTIFICATION ERROR:" << query.lastError().text();
    return ok;
}

bool DatabaseManager::updateLatestNotificationStatus(int businessId, int shiftId, const QString& sendStatus)
{
    QSqlQuery query(db);
    if (shiftId > 0)
    {
        query.prepare(
            "UPDATE notifications "
            "SET send_status = :send_status "
            "WHERE id = ("
            "SELECT id "
            "FROM notifications "
            "WHERE business_id = :business_id "
            "AND shift_id = :shift_id "
            "ORDER BY id DESC "
            "LIMIT 1"
            ")"
            );
        query.bindValue(":shift_id", shiftId);
    }
    else
    {
        query.prepare(
            "UPDATE notifications "
            "SET send_status = :send_status "
            "WHERE id = ("
            "SELECT id "
            "FROM notifications "
            "WHERE business_id = :business_id "
            "ORDER BY id DESC "
            "LIMIT 1"
            ")"
            );
    }

    query.bindValue(":send_status", sendStatus.trimmed());
    query.bindValue(":business_id", businessId);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "UPDATE NOTIFICATION STATUS ERROR:" << query.lastError().text();
    return ok;
}

QList<int> DatabaseManager::getVkRecipientIds(int businessId,
                                              const QString& recipientCode,
                                              int employeeId,
                                              const QString& positionName)
{
    QList<int> result;
    QSqlQuery query(db);

    if (recipientCode == "employee")
    {
        query.prepare(
            "SELECT vk_id "
            "FROM employees "
            "WHERE business_id = :business_id "
            "AND id = :employee_id "
            "AND is_active = 1 "
            "AND vk_id IS NOT NULL "
            "AND TRIM(vk_id) <> ''"
            );
        query.bindValue(":business_id", businessId);
        query.bindValue(":employee_id", employeeId);
    }
    else if (recipientCode == "position")
    {
        query.prepare(
            "SELECT DISTINCT e.vk_id "
            "FROM employees e "
            "LEFT JOIN positions employee_position "
            "ON employee_position.business_id = e.business_id "
            "AND employee_position.name = e.position "
            "LEFT JOIN position_capabilities pc "
            "ON pc.position_id = employee_position.id "
            "LEFT JOIN positions covered_position "
            "ON covered_position.id = pc.covered_position_id "
            "WHERE e.business_id = :business_id "
            "AND e.is_active = 1 "
            "AND e.vk_id IS NOT NULL "
            "AND TRIM(e.vk_id) <> '' "
            "AND (e.position = :position_name OR covered_position.name = :position_name)"
            );
        query.bindValue(":business_id", businessId);
        query.bindValue(":position_name", positionName.trimmed());
    }
    else
    {
        query.prepare(
            "SELECT vk_id "
            "FROM employees "
            "WHERE business_id = :business_id "
            "AND is_active = 1 "
            "AND vk_id IS NOT NULL "
            "AND TRIM(vk_id) <> ''"
            );
        query.bindValue(":business_id", businessId);
    }

    if (!query.exec())
    {
        qDebug() << "GET VK RECIPIENT IDS ERROR:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        bool ok = false;
        const int vkId = query.value("vk_id").toString().trimmed().toInt(&ok);
        if (ok && vkId > 0 && !result.contains(vkId))
            result.append(vkId);
    }

    return result;
}

QList<int> DatabaseManager::getVkRecipientIdsForShiftOpenPositions(int businessId, int shiftId)
{
    QList<int> result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT DISTINCT e.vk_id "
        "FROM shift_open_positions sop "
        "JOIN shifts s ON s.id = sop.shift_id "
        "JOIN employees e ON e.business_id = s.business_id "
        "LEFT JOIN positions employee_position "
        "ON employee_position.business_id = e.business_id "
        "AND employee_position.name = e.position "
        "LEFT JOIN position_capabilities pc "
        "ON pc.position_id = employee_position.id "
        "LEFT JOIN positions covered_position "
        "ON covered_position.id = pc.covered_position_id "
        "WHERE s.business_id = :business_id "
        "AND sop.shift_id = :shift_id "
        "AND sop.employee_count > 0 "
        "AND e.is_active = 1 "
        "AND e.vk_id IS NOT NULL "
        "AND TRIM(e.vk_id) <> '' "
        "AND (e.position = sop.position_name OR covered_position.name = sop.position_name) "
        "AND NOT EXISTS ("
        "SELECT 1 FROM shift_assignments sa "
        "WHERE sa.shift_id = sop.shift_id "
        "AND sa.employee_id = e.id"
        ")"
        );
    query.bindValue(":business_id", businessId);
    query.bindValue(":shift_id", shiftId);

    if (!query.exec())
    {
        qDebug() << "GET OPEN POSITION VK RECIPIENT IDS ERROR:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        bool ok = false;
        const int vkId = query.value("vk_id").toString().trimmed().toInt(&ok);
        if (ok && vkId > 0 && !result.contains(vkId))
            result.append(vkId);
    }

    return result;
}

QList<int> DatabaseManager::getAssignedShiftVkRecipientIds(int shiftId)
{
    QList<int> result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT DISTINCT e.vk_id "
        "FROM shift_assignments sa "
        "JOIN employees e ON e.id = sa.employee_id "
        "WHERE sa.shift_id = :shift_id "
        "AND e.is_active = 1 "
        "AND e.vk_id IS NOT NULL "
        "AND TRIM(e.vk_id) <> ''"
        );
    query.bindValue(":shift_id", shiftId);

    if (!query.exec())
    {
        qDebug() << "GET ASSIGNED SHIFT VK IDS ERROR:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        bool ok = false;
        const int vkId = query.value("vk_id").toString().trimmed().toInt(&ok);
        if (ok && vkId > 0 && !result.contains(vkId))
            result.append(vkId);
    }

    return result;
}

QList<NotificationInfo> DatabaseManager::getNotifications(int businessId)
{
    QList<NotificationInfo> result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, shift_id, recipient_type, recipient_label, notification_type, message_text, "
        "channel, send_status, created_at "
        "FROM notifications "
        "WHERE business_id = ? "
        "ORDER BY created_at DESC, id DESC"
        );
    query.addBindValue(businessId);
    query.exec();

    while (query.next())
    {
        NotificationInfo item;
        item.id = query.value("id").toInt();
        item.shiftId = query.value("shift_id").isNull() ? -1 : query.value("shift_id").toInt();
        item.recipientType = query.value("recipient_type").toString();
        item.recipientLabel = query.value("recipient_label").toString();
        item.notificationType = query.value("notification_type").toString();
        item.messageText = query.value("message_text").toString();
        item.channel = query.value("channel").toString();
        item.sendStatus = query.value("send_status").toString();
        item.createdAt = query.value("created_at").toString();
        result.append(item);
    }

    return result;
}

QList<ShiftResponseInfo> DatabaseManager::getShiftResponses(int businessId)
{
    QList<ShiftResponseInfo> result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT sr.id, sr.shift_id, sr.vk_id, sr.employee_id, e.full_name, sr.position_name, "
        "sr.response_status, sr.response_message, sr.created_at, sr.processed_at "
        "FROM shift_responses sr "
        "LEFT JOIN employees e ON e.id = sr.employee_id "
        "WHERE sr.business_id = ? "
        "ORDER BY sr.created_at DESC, sr.id DESC"
        );
    query.addBindValue(businessId);
    query.exec();

    while (query.next())
    {
        ShiftResponseInfo item;
        item.id = query.value("id").toInt();
        item.shiftId = query.value("shift_id").isNull() ? -1 : query.value("shift_id").toInt();
        item.vkId = query.value("vk_id").toString();
        item.employeeId = query.value("employee_id").isNull() ? -1 : query.value("employee_id").toInt();
        item.employeeName = query.value("full_name").toString();
        item.positionName = query.value("position_name").toString();
        item.responseStatus = query.value("response_status").toString();
        item.responseMessage = query.value("response_message").toString();
        item.createdAt = query.value("created_at").toString();
        item.processedAt = query.value("processed_at").toString();
        result.append(item);
    }

    return result;
}

bool DatabaseManager::saveVkSettings(int businessId, const VkSettingsData& settings)
{
    QSqlQuery existsQuery(db);
    existsQuery.prepare("SELECT id FROM vk_settings WHERE business_id = ?");
    existsQuery.addBindValue(businessId);

    QSqlQuery query(db);
    if (existsQuery.exec() && existsQuery.next())
    {
        query.prepare(
            "UPDATE vk_settings SET "
            "group_id = :group_id, "
            "community_token = :community_token, "
            "backend_url = :backend_url, "
            "is_enabled = :is_enabled "
            "WHERE business_id = :business_id"
            );
    }
    else
    {
        query.prepare(
            "INSERT INTO vk_settings (business_id, group_id, community_token, backend_url, is_enabled) "
            "VALUES (:business_id, :group_id, :community_token, :backend_url, :is_enabled)"
            );
    }

    query.bindValue(":business_id", businessId);
    query.bindValue(":group_id", settings.groupId.trimmed());
    query.bindValue(":community_token", settings.communityToken.trimmed());
    query.bindValue(":backend_url", settings.backendUrl.trimmed());
    query.bindValue(":is_enabled", settings.isEnabled ? 1 : 0);

    const bool ok = query.exec();
    if (!ok)
        qDebug() << "SAVE VK SETTINGS ERROR:" << query.lastError().text();
    return ok;
}

VkSettingsData DatabaseManager::getVkSettings(int businessId)
{
    VkSettingsData settings;
    QSqlQuery query(db);
    query.prepare(
        "SELECT group_id, community_token, backend_url, is_enabled "
        "FROM vk_settings "
        "WHERE business_id = ?"
        );
    query.addBindValue(businessId);

    if (query.exec() && query.next())
    {
        settings.groupId = query.value("group_id").toString();
        settings.communityToken = query.value("community_token").toString();
        settings.backendUrl = query.value("backend_url").toString();
        settings.isEnabled = query.value("is_enabled").toInt() == 1;
    }

    return settings;
}
