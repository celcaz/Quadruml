#include "ProfileStore.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int PinIterations = 10000;

QByteArray hashPin(const QByteArray &salt, const QString &pin)
{
    QByteArray digest = salt + pin.toUtf8();
    for (int i = 0; i < PinIterations; ++i)
        digest = QCryptographicHash::hash(salt + digest, QCryptographicHash::Sha256);
    return digest;
}

Profile profileFrom(const QSqlQuery &query)
{
    Profile profile;
    profile.id = query.value(0).toLongLong();
    profile.name = query.value(1).toString();
    profile.avatar = query.value(2).toString();
    profile.language = query.value(3).toString();
    profile.hasPin = !query.value(4).toString().isEmpty();
    return profile;
}

}

ProfileStore::ProfileStore(QSqlDatabase db)
    : m_db(std::move(db))
{
}

QList<Profile> ProfileStore::all() const
{
    QList<Profile> profiles;
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("SELECT id, nome, avatar, idioma, pin_hash FROM perfis ORDER BY id"));
    while (query.next())
        profiles.append(profileFrom(query));
    return profiles;
}

std::optional<Profile> ProfileStore::find(qint64 id) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id, nome, avatar, idioma, pin_hash FROM perfis WHERE id = ?"));
    query.addBindValue(id);
    if (query.exec() && query.next())
        return profileFrom(query);
    return std::nullopt;
}

qint64 ProfileStore::create(const QString &name, const QString &avatar)
{
    const QString clean = normalizedName(name);
    if (clean.isEmpty())
        return 0;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO perfis (nome, avatar) VALUES (?, ?)"));
    query.addBindValue(clean);
    query.addBindValue(avatar);
    if (!query.exec())
        return 0;
    return query.lastInsertId().toLongLong();
}

bool ProfileStore::rename(qint64 id, const QString &name)
{
    const QString clean = normalizedName(name);
    return !clean.isEmpty() && updateField(id, "nome", clean);
}

bool ProfileStore::setAvatar(qint64 id, const QString &avatar)
{
    return updateField(id, "avatar", avatar);
}

bool ProfileStore::setLanguage(qint64 id, const QString &language)
{
    return updateField(id, "idioma", language);
}

bool ProfileStore::remove(qint64 id)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM perfis WHERE id = ?"));
    query.addBindValue(id);
    return query.exec() && query.numRowsAffected() == 1;
}

bool ProfileStore::setPin(qint64 id, const QString &pin)
{
    if (!isValidPin(pin))
        return false;

    QByteArray salt(16, Qt::Uninitialized);
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32 *>(salt.data()), salt.size() / 4);
    const QString stored = QStringLiteral("v1$") + QString::fromLatin1(salt.toHex()) + QLatin1Char('$')
                           + QString::fromLatin1(hashPin(salt, pin).toHex());
    return updateField(id, "pin_hash", stored);
}

bool ProfileStore::clearPin(qint64 id)
{
    return updateField(id, "pin_hash", QVariant());
}

bool ProfileStore::checkPin(qint64 id, const QString &pin) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT pin_hash FROM perfis WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec() || !query.next())
        return false;

    const QString stored = query.value(0).toString();
    if (stored.isEmpty())
        return true; // perfil sem PIN

    const QStringList parts = stored.split(QLatin1Char('$'));
    if (parts.size() != 3 || parts.at(0) != QLatin1String("v1"))
        return false;
    const QByteArray salt = QByteArray::fromHex(parts.at(1).toLatin1());
    return hashPin(salt, pin).toHex() == parts.at(2).toLatin1();
}

bool ProfileStore::isValidPin(const QString &pin)
{
    if (pin.size() != 4)
        return false;
    for (const QChar c : pin) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
    }
    return true;
}

QString ProfileStore::normalizedName(const QString &name)
{
    return name.simplified().left(MaxNameLength);
}

bool ProfileStore::updateField(qint64 id, const char *column, const QVariant &value)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE perfis SET %1 = ? WHERE id = ?").arg(QLatin1String(column)));
    query.addBindValue(value);
    query.addBindValue(id);
    return query.exec() && query.numRowsAffected() == 1;
}
