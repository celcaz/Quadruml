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
        case GenreRole:
        case ReleaseYearRole: return QString();
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
    case PlatformLabelRole: return platformLabel(game.platform, game.provider);
    case DescriptionRole: return game.description;
    case CoverUrlRole: return game.coverUrl;
    case HeroUrlRole: return game.heroUrl;
    case LogoUrlRole: return game.logoUrl;
    case AccentRole: return game.accent;
    case MinutesPlayedRole: return game.minutesPlayed;
    case GenreRole: return game.genre;
    case ReleaseYearRole: return game.releaseYear;
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
        {GenreRole, "genre"},
        {ReleaseYearRole, "releaseYear"},
    };
}

bool GameListModel::setGames(QList<GameEntry> games)
{
    const bool sameGames = games.size() == m_games.size()
        && std::equal(games.cbegin(), games.cend(), m_games.cbegin(),
                      [](const GameEntry &a, const GameEntry &b) { return a.id == b.id; });

    if (!sameGames) {
        beginResetModel();
        m_games = std::move(games);
        endResetModel();
        return true;
    }

    bool changed = false;
    for (qsizetype row = 0; row < games.size(); ++row) {
        if (games.at(row) == m_games.at(row))
            continue;
        m_games[row] = games.at(row);
        const QModelIndex index = this->index(int(row));
        emit dataChanged(index, index);
        changed = true;
    }
    return changed;
}

QString GameListModel::platformLabel(const QString &platform, const QString &provider)
{
    if (platform == QLatin1String("pc")) {
        if (provider == QLatin1String("steam"))
            return QStringLiteral("Steam");
        if (provider == QLatin1String("epic"))
            return QStringLiteral("Epic Games");
        return QStringLiteral("PC");
    }
    return platform.toUpper(); // ps1 -> PS1
}
