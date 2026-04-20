#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDate>
#include <QList>
#include <QTime>
#include <QStringList>

struct ShiftAssignedEmployeeData
{
    int employeeId = -1;
    QString employeeName;
    QString positionName;
    QString paymentType;
    QString hourlyRate;
    QString fixedRate;
    QString percentRate;
};

struct ShiftOpenPositionData
{
    QString positionName;
    int employeeCount = 1;
    QString paymentType;
    QString hourlyRate;
    QString fixedRate;
    QString percentRate;
};

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
    QSqlQuery getEmployeeById(int employeeId);
    QSqlQuery getPositions(int businessId, bool ascending = true);
    QSqlQuery getPositionById(int positionId);
    QList<int> getCoveredPositionIds(int positionId);
    QStringList getCoveredPositionNames(int positionId);
    QStringList getCoveredPositionNamesByPositionName(int businessId, const QString& positionName);
    bool createEmployee(int businessId,
                        const QString& lastName,
                        const QString& firstName,
                        const QString& middleName,
                        const QDate& birthDate,
                        const QString& gender,
                        const QString& phone,
                        const QString& vkId,
                        const QString& position);
    bool updateEmployee(int employeeId,
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
                        const QString& salaryRate);
    bool createPosition(int businessId,
                        const QString& name,
                        const QString& salaryText,
                        const QList<int>& coveredPositionIds);
    bool updatePosition(int positionId,
                        const QString& name,
                        const QString& salaryText,
                        const QList<int>& coveredPositionIds);
    bool deletePosition(int positionId);
    QStringList getPositionNames(int businessId);
    bool createShift(int businessId,
                     const QDate& shiftDate,
                     const QTime& startTime,
                     const QTime& endTime,
                     const QString& status,
                     const QString& comment,
                     const QList<ShiftAssignedEmployeeData>& assignedEmployees,
                     const QList<ShiftOpenPositionData>& openPositions);
    QSqlQuery getShiftsForList(int businessId, const QDate& fromDate);
    QSqlQuery getAllShifts(int businessId);
    QStringList getShiftAssignedSummary(int shiftId);
    QStringList getShiftOpenPositionsSummary(int shiftId);

private:
    DatabaseManager();
    QSqlDatabase db;
};

#endif
