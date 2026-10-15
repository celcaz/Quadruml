#include "ProfileService.h"

#include <QJSEngine>
#include <QLoggingCategory>

#include "core/ProfileStore.h"

Q_LOGGING_CATEGORY(lcProfiles, "quadrum.profiles")

namespace {
ProfileService *s_instance = nullptr;
}

ProfileService::ProfileService(ProfileStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    reload();
}

ProfileService::~ProfileService()
{
    s_instance = nullptr;
}

ProfileService *ProfileService::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

qint64 ProfileService::createProfile(const QString &name)
{
    const qint64 id = m_store->create(name);
    if (id) {
        qCInfo(lcProfiles) << "perfil criado:" << id;
        reload();
    }
    return id;
}

bool ProfileService::rename(qint64 id, const QString &name)
{
    if (!m_store->rename(id, name))
        return false;
    if (id == m_currentId) {
        m_currentName = ProfileStore::normalizedName(name);
        emit currentChanged();
    }
    reload();
    return true;
}

bool ProfileService::setAvatar(qint64 id, const QString &avatar)
{
    const bool ok = m_store->setAvatar(id, avatar);
    if (ok)
        reload();
    return ok;
}

bool ProfileService::setPin(qint64 id, const QString &pin)
{
    const bool ok = m_store->setPin(id, pin);
    if (ok)
        reload();
    return ok;
}

bool ProfileService::clearPin(qint64 id)
{
    const bool ok = m_store->clearPin(id);
    if (ok)
        reload();
    return ok;
}

bool ProfileService::removeProfile(qint64 id)
{
    if (!m_store->remove(id))
        return false;
    qCInfo(lcProfiles) << "perfil removido:" << id;
    if (id == m_currentId)
        signOut();
    reload();
    return true;
}

bool ProfileService::hasPin(qint64 id) const
{
    const auto profile = m_store->find(id);
    return profile && profile->hasPin;
}

bool ProfileService::signIn(qint64 id, const QString &pin)
{
    const auto profile = m_store->find(id);
    if (!profile)
        return false;
    if (profile->hasPin && !m_store->checkPin(id, pin)) {
        qCInfo(lcProfiles) << "PIN incorreto para o perfil" << id;
        return false;
    }
    m_currentId = profile->id;
    m_currentName = profile->name;
    qCInfo(lcProfiles) << "entrou no perfil" << id;
    emit currentChanged();
    return true;
}

void ProfileService::signOut()
{
    if (!m_currentId)
        return;
    m_currentId = 0;
    m_currentName.clear();
    emit currentChanged();
}

void ProfileService::reload()
{
    m_model.setProfiles(m_store->all());
    emit profilesChanged();
}
