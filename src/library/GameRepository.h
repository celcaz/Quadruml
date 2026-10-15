#pragma once

#include <QList>
#include <QMap>
#include <QSet>
#include <QSqlDatabase>
#include <QStringList>

#include "ScannedGame.h"

struct StoredGame
{
    qint64 id = 0;
    QString platform;
    QString provider;
    QString externalId;
    QString title;
    bool identificationPending = false;
    QString coverPath;   // capa vertical (ex.: Steam)
    QString squarePath;  // capa quadrada (ícone estilo PS5)
    QString heroPath;
    QString logoPath;
    QString description;
    QString genre;
    QString releaseDate;
};

// Jogo que ainda precisa de artes ou descrição da internet.
struct MetadataTask
{
    qint64 gameId = 0;
    QString platform;
    QString provider;
    QString externalId;
    QString title;
    int sgdbId = 0;
    bool needArtwork = false;
    bool needSquare = false;
    bool needHero = false;
    bool needLogo = false;
    bool needInfo = false;
};

// Jogos e arquivos no banco (tabelas jogos e jogo_arquivos).
class GameRepository
{
public:
    explicit GameRepository(QSqlDatabase db);

    // O arquivo já está no banco com o mesmo tamanho e data? Então não precisa
    // ser aberto de novo (varredura incremental).
    bool isUnchanged(const QString &path, qint64 size, const QDateTime &modified) const;

    // Cria ou atualiza o jogo (por provedor + código), os seus arquivos e artes.
    qint64 upsert(const ScannedGame &game);

    // Atualiza só as artes de um jogo já existente (ex.: a Steam baixou a arte
    // depois). Artes escolhidas pelo usuário nunca são trocadas.
    void updateMedia(const QString &provider, const QString &externalId, const QMap<QString, QString> &media);

    // Remove do banco os arquivos do provedor que estão dentro de "roots" mas
    // não foram vistos nesta varredura, e os jogos que ficaram sem arquivo.
    // Pastas que não existem (disco desconectado) são ignoradas: nada é apagado.
    int removeMissing(const QString &provider, const QStringList &roots, const QSet<QString> &seenPaths);

    // --- Metadados (artes e descrições da internet) ---

    // Jogos sem tentativa ainda, ou cuja última tentativa falhou há mais de
    // retryAfterDays dias.
    QList<MetadataTask> metadataTasks(int retryAfterDays = 7) const;
    void setMedia(qint64 gameId, const QString &type, const QString &path, const QString &origin);
    void setInfo(qint64 gameId, const QString &description, const QString &genre, const QString &releaseDate,
                 const QString &source);
    // result: "ok", "nao_encontrado" ou "erro"
    void markArtwork(qint64 gameId, const QString &result, int sgdbId = 0);
    void markInfo(qint64 gameId, const QString &result);

    QList<StoredGame> recent(int limit) const;
    QMap<QString, int> countByPlatform() const;
    int pendingCount() const;

    static QString sortTitle(const QString &title);

private:
    void writeMedia(qint64 gameId, const QMap<QString, QString> &media, const QString &origin);

    QSqlDatabase m_db;
};
