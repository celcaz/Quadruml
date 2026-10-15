#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "ProfileModel.h"

class ProfileStore;
class QQmlEngine;
class QJSEngine;

// Perfis para as telas: lista, perfil ativo, criar, editar, PIN e entrar.
class ProfileService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(ProfileModel *model READ model CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY profilesChanged)
    Q_PROPERTY(qint64 currentId READ currentId NOTIFY currentChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentChanged)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    ProfileService(ProfileStore *store, QObject *parent);
    ~ProfileService() override;

    static ProfileService *create(QQmlEngine *, QJSEngine *);

    ProfileModel *model() { return &m_model; }
    int count() const { return m_model.rowCount(); }
    qint64 currentId() const { return m_currentId; }
    QString currentName() const { return m_currentName; }

    Q_INVOKABLE qint64 createProfile(const QString &name);
    Q_INVOKABLE bool rename(qint64 id, const QString &name);
    Q_INVOKABLE bool setAvatar(qint64 id, const QString &avatar);
    Q_INVOKABLE bool setPin(qint64 id, const QString &pin);
    Q_INVOKABLE bool clearPin(qint64 id);
    Q_INVOKABLE bool removeProfile(qint64 id);
    Q_INVOKABLE bool hasPin(qint64 id) const;

    // Entra no perfil; para perfis com PIN, o PIN precisa estar certo.
    Q_INVOKABLE bool signIn(qint64 id, const QString &pin = {});
    Q_INVOKABLE void signOut();

signals:
    void profilesChanged();
    void currentChanged();

private:
    void reload();

    ProfileStore *m_store;
    ProfileModel m_model;
    qint64 m_currentId = 0;
    QString m_currentName;
};
