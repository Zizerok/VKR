#ifndef SECURITY_H
#define SECURITY_H

#include <QString>

class Security
{
public:
    static QString generateSalt();
    static QString hashPassword(const QString& password, const QString& salt);
};

#endif
