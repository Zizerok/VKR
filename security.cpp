#include "security.h"

#include <QRandomGenerator>
#include <QCryptographicHash>

QString Security::generateSalt()
{
    QString salt;
    for(int i = 0; i < 16; i++)
    {
        int value = QRandomGenerator::global()->bounded(256);
        salt.append(QString::number(value, 16));
    }
    return salt;
}

QString Security::hashPassword(const QString& password, const QString& salt)
{
    QString combined = password + salt;

    QByteArray hash = QCryptographicHash::hash(
        combined.toUtf8(),
        QCryptographicHash::Sha256
        );

    return hash.toHex();
}
