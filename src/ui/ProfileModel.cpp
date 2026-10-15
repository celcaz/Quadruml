#include "ProfileModel.h"

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_profiles.size());
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const Profile &profile = m_profiles.at(index.row());
    switch (role) {
    case IdRole: return profile.id;
    case NameRole:
    case Qt::DisplayRole: return profile.name;
    case AvatarRole: return profile.avatar;
    case HasPinRole: return profile.hasPin;
    }
    return {};
}

QHash<int, QByteArray> ProfileModel::roleNames() const
{
    return {
        {IdRole, "profileId"},
        {NameRole, "name"},
        {AvatarRole, "avatar"},
        {HasPinRole, "hasPin"},
    };
}

void ProfileModel::setProfiles(QList<Profile> profiles)
{
    beginResetModel();
    m_profiles = std::move(profiles);
    endResetModel();
}
