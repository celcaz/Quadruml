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
    QString provider; // "ps2", "steam", ... (para jogos de PC, o nome da loja)
    QString description;
    QString genre;
    QString releaseYear;
    QUrl coverUrl;
    QUrl heroUrl;
    QUrl logoUrl;
    QColor accent;    // cor provisória enquanto não há capa/fundo
    int minutesPlayed = 0;

    bool operator==(const GameEntry &) const = default;
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
        GenreRole,
        ReleaseYearRole,
    };

    explicit GameListModel(bool includeLibraryTile, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Atualiza a lista mexendo o mínimo possível na tela:
    //  - mesmos jogos na mesma ordem: avisa só as linhas que mudaram (as capas
    //    continuam existindo; só a que mudou troca de imagem);
    //  - nada mudou: não faz nada;
    //  - entrou ou saiu jogo: recria a lista.
    // Devolve se algo mudou.
    bool setGames(QList<GameEntry> games);

    // "PS2", "PS3"...; para PC, o nome da loja ("Steam").
    static QString platformLabel(const QString &platform, const QString &provider = {});

private:
    QList<GameEntry> m_games;
    bool m_includeLibraryTile;
};
