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

struct EmployeePaymentSummary
{
    int employeeId = -1;
    QString employeeName;
    double totalAmount = 0.0;
    int unpaidAssignments = 0;
};

struct ShiftPaymentInfo
{
    int assignmentId = -1;
    int shiftId = -1;
    int employeeId = -1;
    QString employeeName;
    QString shiftDate;
    QString timeRange;
    QString positionName;
    QString paymentType;
    QString hourlyRate;
    QString fixedRate;
    QString percentRate;
    QString revenueAmount;
    QString paidAt;
    bool isPaid = false;
};

struct NotificationInfo
{
    int id = -1;
    int shiftId = -1;
    QString recipientType;
    QString recipientLabel;
    QString notificationType;
    QString messageText;
    QString channel;
    QString sendStatus;
    QString createdAt;
};

struct ShiftResponseInfo
{
    int id = -1;
    int shiftId = -1;
    int employeeId = -1;
    QString vkId;
    QString employeeName;
    QString positionName;
    QString responseStatus;
    QString responseMessage;
    QString createdAt;
    QString processedAt;
};

struct VkSettingsData
{
    QString groupId;
    QString communityToken;
    QString backendUrl;
    bool isEnabled = false;
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
    int getLastShiftIdForBusiness(int businessId);
    QSqlQuery getShiftById(int shiftId);
    QList<ShiftAssignedEmployeeData> getShiftAssignments(int shiftId);
    QList<ShiftOpenPositionData> getShiftOpenPositions(int shiftId);
    bool updateShift(int shiftId,
                     const QDate& shiftDate,
                     const QTime& startTime,
                     const QTime& endTime,
                     const QString& status,
                     const QString& comment,
                     const QList<ShiftAssignedEmployeeData>& assignedEmployees,
                     const QList<ShiftOpenPositionData>& openPositions);
    bool deleteShift(int shiftId);
    QSqlQuery getShiftsForPeriod(int businessId, const QDate& startDate, const QDate& endDate);
    QSqlQuery getShiftsForList(int businessId, const QDate& fromDate);
    QSqlQuery getShiftsWithoutNewShiftNotification(int businessId);
    QSqlQuery getAllShifts(int businessId);
    QStringList getShiftAssignedSummary(int shiftId);
    QStringList getShiftOpenPositionsSummary(int shiftId);
    QList<EmployeePaymentSummary> getEmployeePaymentSummaries(int businessId);
    QList<ShiftPaymentInfo> getEmployeeShiftPayments(int businessId, int employeeId);
    bool updateShiftAssignmentRevenue(int assignmentId, const QString& revenueAmount);
    bool markShiftAssignmentPaid(int assignmentId);
    bool markAllEmployeeAssignmentsPaid(int businessId, int employeeId);
    bool createNotification(int businessId,
                            int shiftId,
                            const QString& recipientType,
                            const QString& recipientLabel,
                            const QString& notificationType,
                            const QString& messageText,
                            const QString& channel,
                            const QString& sendStatus);
    bool updateLatestNotificationStatus(int businessId, int shiftId, const QString& sendStatus);
    QList<int> getVkRecipientIds(int businessId,
                                 const QString& recipientCode,
                                 int employeeId,
                                 const QString& positionName);
    QList<int> getVkRecipientIdsForShiftOpenPositions(int businessId, int shiftId);
    QList<int> getAssignedShiftVkRecipientIds(int shiftId);
    QList<NotificationInfo> getNotifications(int businessId);
    QList<ShiftResponseInfo> getShiftResponses(int businessId);
    bool saveVkSettings(int businessId, const VkSettingsData& settings);
    VkSettingsData getVkSettings(int businessId);

private:
    DatabaseManager();
    QSqlDatabase db;
};

#endif
