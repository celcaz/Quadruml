#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include "core/ProfileStore.h"

// Lista de perfis no formato que as telas QML consomem (ListView, Repeater...).
class ProfileModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ProfileService.model")

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        AvatarRole,
        HasPinRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setProfiles(QList<Profile> profiles);

private:
    QList<Profile> m_profiles;
};
