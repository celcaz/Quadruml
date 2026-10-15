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
    QString externalId;
    QString title;
    bool identificationPending = false;
};

// Jogos e arquivos no banco (tabelas jogos e jogo_arquivos).
class GameRepository
{
public:
    explicit GameRepository(QSqlDatabase db);

    // O arquivo já está no banco com o mesmo tamanho e data? Então não precisa
    // ser aberto de novo (varredura incremental).
    bool isUnchanged(const QString &path, qint64 size, const QDateTime &modified) const;

    // Cria ou atualiza o jogo (por provedor + código) e os seus arquivos.
    qint64 upsert(const ScannedGame &game);

    // Remove do banco os arquivos do provedor que estão dentro de "roots" mas
    // não foram vistos nesta varredura, e os jogos que ficaram sem arquivo.
    // Pastas que não existem (disco desconectado) são ignoradas: nada é apagado.
    int removeMissing(const QString &provider, const QStringList &roots, const QSet<QString> &seenPaths);

    QList<StoredGame> recent(int limit) const;
    QMap<QString, int> countByPlatform() const;
    int pendingCount() const;

    static QString sortTitle(const QString &title);

private:
    QSqlDatabase m_db;
};
