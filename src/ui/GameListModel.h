#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

struct GameEntry
{
    qint64 id = 0;
    QString title;
    QString platform; // "ps1", "ps2", "ps3", "ps4", "pc"
    QString description;
    QUrl coverUrl;
    QUrl heroUrl;
    QUrl logoUrl;
    QColor accent;    // cor provisória enquanto não há capa/fundo
    int minutesPlayed = 0;
};

// Lista de jogos para as telas (fileira da tela inicial, grade da biblioteca).
// Com includeLibraryTile, acrescenta no fim o item "Biblioteca" da fileira.
class GameListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use LibraryService")

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        KindRole,
        TitleRole,
        PlatformRole,
        PlatformLabelRole,
        DescriptionRole,
        CoverUrlRole,
        HeroUrlRole,
        LogoUrlRole,
        AccentRole,
        MinutesPlayedRole,
    };

    explicit GameListModel(bool includeLibraryTile, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setGames(QList<GameEntry> games);

    static QString platformLabel(const QString &platform);

private:
    QList<GameEntry> m_games;
    bool m_includeLibraryTile;
};
