#include "GameListModel.h"

GameListModel::GameListModel(bool includeLibraryTile, QObject *parent)
    : QAbstractListModel(parent)
    , m_includeLibraryTile(includeLibraryTile)
{
}

int GameListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return int(m_games.size()) + (m_includeLibraryTile ? 1 : 0);
}

QVariant GameListModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    if (index.row() >= m_games.size()) {
        // Item "Biblioteca" no fim da fileira.
        switch (role) {
        case KindRole: return QStringLiteral("library");
        case IdRole: return qint64(0);
        case MinutesPlayedRole: return 0;
        case AccentRole: return QColor();
        case CoverUrlRole:
        case HeroUrlRole:
        case LogoUrlRole: return QUrl();
        default: return QString();
        }
    }

    const GameEntry &game = m_games.at(index.row());
    switch (role) {
    case IdRole: return game.id;
    case KindRole: return QStringLiteral("game");
    case TitleRole:
    case Qt::DisplayRole: return game.title;
    case PlatformRole: return game.platform;
    case PlatformLabelRole: return platformLabel(game.platform);
    case DescriptionRole: return game.description;
    case CoverUrlRole: return game.coverUrl;
    case HeroUrlRole: return game.heroUrl;
    case LogoUrlRole: return game.logoUrl;
    case AccentRole: return game.accent;
    case MinutesPlayedRole: return game.minutesPlayed;
    }
    return {};
}

QHash<int, QByteArray> GameListModel::roleNames() const
{
    return {
        {IdRole, "gameId"},
        {KindRole, "kind"},
        {TitleRole, "title"},
        {PlatformRole, "platform"},
        {PlatformLabelRole, "platformLabel"},
        {DescriptionRole, "description"},
        {CoverUrlRole, "coverUrl"},
        {HeroUrlRole, "heroUrl"},
        {LogoUrlRole, "logoUrl"},
        {AccentRole, "accent"},
        {MinutesPlayedRole, "minutesPlayed"},
    };
}

void GameListModel::setGames(QList<GameEntry> games)
{
    beginResetModel();
    m_games = std::move(games);
    endResetModel();
}

QString GameListModel::platformLabel(const QString &platform)
{
    if (platform == QLatin1String("pc"))
        return QStringLiteral("PC");
    return platform.toUpper(); // ps1 -> PS1
}
