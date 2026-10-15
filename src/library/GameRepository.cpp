#include "GameRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>

GameRepository::GameRepository(QSqlDatabase db)
    : m_db(std::move(db))
{
}

bool GameRepository::isUnchanged(const QString &path, qint64 size, const QDateTime &modified) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT tamanho, modificado_em FROM jogo_arquivos WHERE caminho = ?"));
    query.addBindValue(path);
    if (!query.exec() || !query.next())
        return false;
    return query.value(0).toLongLong() == size
           && query.value(1).toString() == modified.toUTC().toString(Qt::ISODate);
}

qint64 GameRepository::upsert(const ScannedGame &game)
{
    m_db.transaction();
    QSqlQuery query(m_db);

    query.prepare(QStringLiteral("SELECT id FROM jogos WHERE provedor = ? AND id_externo = ?"));
    query.addBindValue(game.provider);
    query.addBindValue(game.externalId);
    qint64 id = 0;
    if (query.exec() && query.next())
        id = query.value(0).toLongLong();
    query.finish();

    if (id) {
        query.prepare(QStringLiteral(
            "UPDATE jogos SET titulo = ?, titulo_ordenacao = ?, plataforma = ?,"
            " identificacao_pendente = ?, atualizado_em = datetime('now') WHERE id = ?"));
        query.addBindValue(game.title);
        query.addBindValue(sortTitle(game.title));
        query.addBindValue(game.platform);
        query.addBindValue(game.identificationPending ? 1 : 0);
        query.addBindValue(id);
        query.exec();
    } else {
        query.prepare(QStringLiteral(
            "INSERT INTO jogos (plataforma, provedor, id_externo, titulo, titulo_ordenacao, identificacao_pendente)"
            " VALUES (?, ?, ?, ?, ?, ?)"));
        query.addBindValue(game.platform);
        query.addBindValue(game.provider);
        query.addBindValue(game.externalId);
        query.addBindValue(game.title);
        query.addBindValue(sortTitle(game.title));
        query.addBindValue(game.identificationPending ? 1 : 0);
        query.exec();
        id = query.lastInsertId().toLongLong();
    }

    for (const ScannedFile &file : game.files) {
        query.prepare(QStringLiteral(
            "INSERT INTO jogo_arquivos (jogo_id, caminho, formato, tamanho, modificado_em, disco)"
            " VALUES (?, ?, ?, ?, ?, ?)"
            " ON CONFLICT(caminho) DO UPDATE SET jogo_id = excluded.jogo_id, formato = excluded.formato,"
            " tamanho = excluded.tamanho, modificado_em = excluded.modificado_em, disco = excluded.disco"));
        query.addBindValue(id);
        query.addBindValue(file.path);
        query.addBindValue(file.format);
        query.addBindValue(file.size);
        query.addBindValue(file.modified.toUTC().toString(Qt::ISODate));
        query.addBindValue(file.disc);
        query.exec();
    }
    query.finish();
    m_db.commit();
    return id;
}

int GameRepository::removeMissing(const QString &provider, const QStringList &roots, const QSet<QString> &seenPaths)
{
    QStringList existingRoots;
    for (const QString &root : roots) {
        if (QFileInfo(root).isDir())
            existingRoots.append(QDir::cleanPath(QDir::fromNativeSeparators(root)) + QLatin1Char('/'));
    }
    if (existingRoots.isEmpty())
        return 0;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT a.id, a.caminho FROM jogo_arquivos a JOIN jogos j ON j.id = a.jogo_id WHERE j.provedor = ?"));
    query.addBindValue(provider);
    query.exec();

    QList<qint64> toRemove;
    while (query.next()) {
        const QString path = query.value(1).toString();
        const bool underScannedRoot = std::any_of(existingRoots.cbegin(), existingRoots.cend(),
            [&](const QString &root) { return path.startsWith(root, Qt::CaseInsensitive); });
        if (underScannedRoot && !seenPaths.contains(path))
            toRemove.append(query.value(0).toLongLong());
    }
    query.finish();
    if (toRemove.isEmpty())
        return 0;

    m_db.transaction();
    for (const qint64 fileId : toRemove) {
        query.prepare(QStringLiteral("DELETE FROM jogo_arquivos WHERE id = ?"));
        query.addBindValue(fileId);
        query.exec();
    }
    query.exec(QStringLiteral(
        "DELETE FROM jogos WHERE id NOT IN (SELECT DISTINCT jogo_id FROM jogo_arquivos)"));
    query.finish();
    m_db.commit();
    return int(toRemove.size());
}

QList<StoredGame> GameRepository::recent(int limit) const
{
    QList<StoredGame> games;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, plataforma, id_externo, titulo, identificacao_pendente FROM jogos"
        " ORDER BY adicionado_em DESC, id DESC LIMIT ?"));
    query.addBindValue(limit);
    query.exec();
    while (query.next()) {
        StoredGame game;
        game.id = query.value(0).toLongLong();
        game.platform = query.value(1).toString();
        game.externalId = query.value(2).toString();
        game.title = query.value(3).toString();
        game.identificationPending = query.value(4).toBool();
        games.append(game);
    }
    return games;
}

QMap<QString, int> GameRepository::countByPlatform() const
{
    QMap<QString, int> counts;
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("SELECT plataforma, COUNT(*) FROM jogos GROUP BY plataforma"));
    while (query.next())
        counts.insert(query.value(0).toString(), query.value(1).toInt());
    return counts;
}

int GameRepository::pendingCount() const
{
    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM jogos WHERE identificacao_pendente = 1")) && query.next())
        return query.value(0).toInt();
    return 0;
}

QString GameRepository::sortTitle(const QString &title)
{
    // Ordenação sem diferenciar maiúsculas e sem acentos: "Ébano" junto de "ebano".
    QString normalized = title.normalized(QString::NormalizationForm_D).toLower();
    normalized.removeIf([](QChar c) { return c.category() == QChar::Mark_NonSpacing; });
    return normalized.trimmed();
}
