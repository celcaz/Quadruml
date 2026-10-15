#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include <optional>

struct Profile
{
    qint64 id = 0;
    QString name;
    QString avatar;
    QString language;
    bool hasPin = false;
};

// Perfis de usuário no banco. Não conhece QML: a interface usa ProfileService.
class ProfileStore
{
public:
    static constexpr int MaxNameLength = 32;

    explicit ProfileStore(QSqlDatabase db);

    QList<Profile> all() const;
    std::optional<Profile> find(qint64 id) const;

    // Devolve o id do novo perfil, ou 0 se o nome for inválido.
    qint64 create(const QString &name, const QString &avatar = {});
    bool rename(qint64 id, const QString &name);
    bool setAvatar(qint64 id, const QString &avatar);
    bool setLanguage(qint64 id, const QString &language);
    bool remove(qint64 id);

    // PIN de 4 dígitos. Serve para separar perfis em casa, não como senha forte.
    // Guardado só como hash com sal; o PIN em si nunca vai para o banco.
    bool setPin(qint64 id, const QString &pin);
    bool clearPin(qint64 id);
    bool checkPin(qint64 id, const QString &pin) const;

    static bool isValidPin(const QString &pin);
    static QString normalizedName(const QString &name);

private:
    bool updateField(qint64 id, const char *column, const QVariant &value);

    QSqlDatabase m_db;
};
