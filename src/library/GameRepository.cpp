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
    writeMedia(id, game.media, game.provider);
    m_db.commit();
    return id;
}

void GameRepository::updateMedia(const QString &provider, const QString &externalId,
                                 const QMap<QString, QString> &media)
{
    if (media.isEmpty())
        return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id FROM jogos WHERE provedor = ? AND id_externo = ?"));
    query.addBindValue(provider);
    query.addBindValue(externalId);
    if (!query.exec() || !query.next())
        return;
    const qint64 id = query.value(0).toLongLong();
    query.finish();
    writeMedia(id, media, provider);
}

void GameRepository::writeMedia(qint64 gameId, const QMap<QString, QString> &media, const QString &origin)
{
    QSqlQuery query(m_db);
    for (auto it = media.cbegin(); it != media.cend(); ++it) {
        query.prepare(QStringLiteral(
            "INSERT INTO jogo_midia (jogo_id, tipo, caminho_local, origem) VALUES (?, ?, ?, ?)"
            " ON CONFLICT(jogo_id, tipo) DO UPDATE SET caminho_local = excluded.caminho_local,"
            " origem = excluded.origem WHERE jogo_midia.escolhida_pelo_usuario = 0"));
        query.addBindValue(gameId);
        query.addBindValue(it.key());
        query.addBindValue(it.value());
        query.addBindValue(origin);
        query.exec();
    }
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
        "SELECT j.id, j.plataforma, j.provedor, j.id_externo, j.titulo, j.identificacao_pendente,"
        " (SELECT caminho_local FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'capa'),"
        " (SELECT caminho_local FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'quadrada'),"
        " (SELECT caminho_local FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'fundo'),"
        " (SELECT caminho_local FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'logo'),"
        " i.descricao, i.genero, i.lancamento"
        " FROM jogos j LEFT JOIN jogo_info i ON i.jogo_id = j.id"
        " ORDER BY j.adicionado_em DESC, j.id DESC LIMIT ?"));
    query.addBindValue(limit);
    query.exec();
    while (query.next()) {
        StoredGame game;
        game.id = query.value(0).toLongLong();
        game.platform = query.value(1).toString();
        game.provider = query.value(2).toString();
        game.externalId = query.value(3).toString();
        game.title = query.value(4).toString();
        game.identificationPending = query.value(5).toBool();
        game.coverPath = query.value(6).toString();
        game.squarePath = query.value(7).toString();
        game.heroPath = query.value(8).toString();
        game.logoPath = query.value(9).toString();
        game.description = query.value(10).toString();
        game.genre = query.value(11).toString();
        game.releaseDate = query.value(12).toString();
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

QList<MetadataTask> GameRepository::metadataTasks(int retryAfterDays) const
{
    // "Precisa" = nunca tentado, ou falhou e já passou o prazo para tentar de novo.
    const QString due = QStringLiteral(
        "(m.%1_em IS NULL OR (m.%1_resultado <> 'ok' AND m.%1_em < datetime('now', '-%2 days')))");
    const QString artworkDue = due.arg(QStringLiteral("artes")).arg(retryAfterDays);
    const QString infoDue = due.arg(QStringLiteral("info")).arg(retryAfterDays);

    QSqlQuery query(m_db);
    query.exec(QStringLiteral(
        "SELECT j.id, j.plataforma, j.provedor, j.id_externo, j.titulo, COALESCE(m.sgdb_id, 0),"
        " %1 AS artes, %2 AS info,"
        " EXISTS(SELECT 1 FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'quadrada'),"
        " EXISTS(SELECT 1 FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'fundo'),"
        " EXISTS(SELECT 1 FROM jogo_midia WHERE jogo_id = j.id AND tipo = 'logo')"
        " FROM jogos j LEFT JOIN metadados m ON m.jogo_id = j.id"
        " WHERE j.identificacao_pendente = 0 AND (%1 OR %2)"
        " ORDER BY j.adicionado_em DESC").arg(artworkDue, infoDue));

    QList<MetadataTask> tasks;
    while (query.next()) {
        MetadataTask task;
        task.gameId = query.value(0).toLongLong();
        task.platform = query.value(1).toString();
        task.provider = query.value(2).toString();
        task.externalId = query.value(3).toString();
        task.title = query.value(4).toString();
        task.sgdbId = query.value(5).toInt();
        task.needArtwork = query.value(6).toBool();
        task.needInfo = query.value(7).toBool();
        task.needSquare = task.needArtwork && !query.value(8).toBool();
        task.needHero = task.needArtwork && !query.value(9).toBool();
        task.needLogo = task.needArtwork && !query.value(10).toBool();
        tasks.append(task);
    }
    return tasks;
}

void GameRepository::setMedia(qint64 gameId, const QString &type, const QString &path, const QString &origin)
{
    writeMedia(gameId, {{type, path}}, origin);
}

void GameRepository::setInfo(qint64 gameId, const QString &description, const QString &genre,
                             const QString &releaseDate, const QString &source)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO jogo_info (jogo_id, descricao, genero, lancamento, fonte) VALUES (?, ?, ?, ?, ?)"
        " ON CONFLICT(jogo_id) DO UPDATE SET descricao = excluded.descricao, genero = excluded.genero,"
        " lancamento = excluded.lancamento, fonte = excluded.fonte"));
    query.addBindValue(gameId);
    query.addBindValue(description);
    query.addBindValue(genre);
    query.addBindValue(releaseDate);
    query.addBindValue(source);
    query.exec();
}

void GameRepository::markArtwork(qint64 gameId, const QString &result, int sgdbId)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO metadados (jogo_id, sgdb_id, artes_em, artes_resultado) VALUES (?, ?, datetime('now'), ?)"
        " ON CONFLICT(jogo_id) DO UPDATE SET artes_em = excluded.artes_em,"
        " artes_resultado = excluded.artes_resultado,"
        " sgdb_id = CASE WHEN excluded.sgdb_id > 0 THEN excluded.sgdb_id ELSE metadados.sgdb_id END"));
    query.addBindValue(gameId);
    query.addBindValue(sgdbId);
    query.addBindValue(result);
    query.exec();
}

void GameRepository::markInfo(qint64 gameId, const QString &result)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO metadados (jogo_id, info_em, info_resultado) VALUES (?, datetime('now'), ?)"
        " ON CONFLICT(jogo_id) DO UPDATE SET info_em = excluded.info_em, info_resultado = excluded.info_resultado"));
    query.addBindValue(gameId);
    query.addBindValue(result);
    query.exec();
}
