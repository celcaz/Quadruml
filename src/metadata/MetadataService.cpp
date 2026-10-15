#include "MetadataService.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QUrlQuery>

#include "AppInfo.h"
#include "MetadataParsing.h"

Q_LOGGING_CATEGORY(lcMeta, "quadrum.metadata")

namespace {

constexpr int DelayBetweenGamesMs = 300;
const QString SgdbBase = QStringLiteral("https://www.steamgriddb.com/api/v2");

QByteArray userAgent()
{
    return QByteArray(AppInfo::Id) + '/' + AppInfo::Version;
}

}

MetadataService::MetadataService(GameRepository *repository, MetadataConfig config, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_config(std::move(config))
{
    m_network.setTransferTimeout(20000);
}

void MetadataService::start()
{
    if (m_running)
        return;
    // Só os jogos para os quais há algo a fazer com as chaves configuradas.
    const bool hasSgdb = !m_config.steamGridDbKey.isEmpty();
    const bool hasIgdb = !m_config.twitchClientId.isEmpty() && !m_config.twitchClientSecret.isEmpty();
    m_tasks.clear();
    for (const MetadataTask &task : m_repository->metadataTasks()) {
        const bool info = task.needInfo && (task.provider == QLatin1String("steam") || hasIgdb);
        if (info || (task.needArtwork && hasSgdb))
            m_tasks.append(task);
    }
    if (m_tasks.isEmpty())
        return;
    m_running = true;
    m_processed = 0;
    qCInfo(lcMeta) << m_tasks.size() << "jogo(s) com artes ou descrição pendentes";
    nextTask();
}

void MetadataService::nextTask()
{
    if (m_tasks.isEmpty()) {
        m_running = false;
        qCInfo(lcMeta) << "metadados concluídos:" << m_processed << "jogo(s)";
        emit finished(m_processed);
        return;
    }

    const MetadataTask task = m_tasks.takeFirst();
    m_steps.clear();

    if (task.needInfo) {
        if (task.provider == QLatin1String("steam"))
            m_steps.append([this, task](Done done) { steamInfo(task, done); });
        else if (!m_config.twitchClientId.isEmpty() && !m_config.twitchClientSecret.isEmpty())
            m_steps.append([this, task](Done done) { igdbInfo(task, done); });
    }
    if (task.needArtwork && !m_config.steamGridDbKey.isEmpty())
        m_steps.append([this, task](Done done) { artwork(task, done); });

    if (m_steps.isEmpty()) {
        nextTask(); // nada a fazer com as chaves configuradas
        return;
    }

    m_steps.append([this, task](Done done) {
        ++m_processed;
        emit gameUpdated(task.gameId);
        done();
    });
    runSteps();
}

void MetadataService::runSteps()
{
    if (m_steps.isEmpty()) {
        QTimer::singleShot(DelayBetweenGamesMs, this, &MetadataService::nextTask);
        return;
    }
    const Step step = m_steps.takeFirst();
    step([this] { runSteps(); });
}

// --- Descrições -------------------------------------------------------------

void MetadataService::steamInfo(const MetadataTask &task, Done done)
{
    QUrl url(QStringLiteral("https://store.steampowered.com/api/appdetails"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("appids"), task.externalId);
    query.addQueryItem(QStringLiteral("l"), QStringLiteral("brazilian"));
    url.setQuery(query);

    get(url, {}, [this, task, done](int status, const QByteArray &body) {
        if (status != 200) {
            m_repository->markInfo(task.gameId, QStringLiteral("erro"));
            done();
            return;
        }
        const auto info = MetadataParsing::parseSteamAppDetails(body, task.externalId);
        if (info.found)
            m_repository->setInfo(task.gameId, info.description, info.genre, info.releaseDate, QStringLiteral("steam"));
        m_repository->markInfo(task.gameId, info.found ? QStringLiteral("ok") : QStringLiteral("nao_encontrado"));
        qCInfo(lcMeta) << "descrição Steam" << task.title << (info.found ? "ok" : "não encontrada");
        done();
    });
}

void MetadataService::igdbInfo(const MetadataTask &task, Done done)
{
    withIgdbToken([this, task, done](bool ok) {
        if (!ok) {
            done();
            return;
        }
        const QString title = MetadataParsing::cleanTitle(task.title);
        QString query = QStringLiteral("search \"%1\"; fields name,summary,first_release_date,genres.name; limit 5;")
                            .arg(QString(title).replace(QLatin1Char('"'), QString()));
        const int platform = MetadataParsing::igdbPlatform(task.platform);
        if (platform)
            query.insert(query.indexOf(QLatin1String(" limit")), QStringLiteral(" where platforms = (%1);").arg(platform));

        post(QUrl(QStringLiteral("https://api.igdb.com/v4/games")),
             {{"Client-ID", m_config.twitchClientId.toUtf8()}, {"Authorization", "Bearer " + m_igdbToken.toUtf8()}},
             query.toUtf8(), [this, task, done](int status, const QByteArray &body) {
                 if (status != 200) {
                     m_repository->markInfo(task.gameId, QStringLiteral("erro"));
                     done();
                     return;
                 }
                 const auto info = MetadataParsing::parseIgdbGames(body, task.title);
                 if (info.found)
                     m_repository->setInfo(task.gameId, info.description, info.genre, info.releaseDate, QStringLiteral("igdb"));
                 m_repository->markInfo(task.gameId, info.found ? QStringLiteral("ok") : QStringLiteral("nao_encontrado"));
                 qCInfo(lcMeta) << "descrição IGDB" << task.title << (info.found ? "ok" : "não encontrada");
                 done();
             });
    });
}

void MetadataService::withIgdbToken(std::function<void(bool ok)> callback)
{
    if (!m_igdbToken.isEmpty()) {
        callback(true);
        return;
    }
    QUrl url(QStringLiteral("https://id.twitch.tv/oauth2/token"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), m_config.twitchClientId);
    query.addQueryItem(QStringLiteral("client_secret"), m_config.twitchClientSecret);
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    url.setQuery(query);

    post(url, {}, {}, [this, callback](int status, const QByteArray &body) {
        m_igdbToken = QJsonDocument::fromJson(body).object().value(QStringLiteral("access_token")).toString();
        if (status != 200 || m_igdbToken.isEmpty()) {
            qCWarning(lcMeta) << "IGDB: não foi possível autenticar na Twitch (status" << status
                              << ") - confira twitch_client_id e twitch_client_secret";
            // Sem token, não adianta tentar os outros jogos nesta sessão.
            m_config.twitchClientId.clear();
            callback(false);
            return;
        }
        callback(true);
    });
}

// --- Artes ------------------------------------------------------------------

QList<QPair<QByteArray, QByteArray>> MetadataService::sgdbHeaders() const
{
    return {{"Authorization", "Bearer " + m_config.steamGridDbKey.toUtf8()}};
}

void MetadataService::artwork(const MetadataTask &task, Done done)
{
    if (task.sgdbId > 0) {
        fetchSgdbImages(task, task.sgdbId, done);
        return;
    }

    // Steam: busca direta pelo AppID. Consoles: busca pelo nome.
    const bool steam = task.provider == QLatin1String("steam");
    const QUrl url = steam
        ? QUrl(SgdbBase + QStringLiteral("/games/steam/") + task.externalId)
        : QUrl(SgdbBase + QStringLiteral("/search/autocomplete/")
               + QString::fromUtf8(QUrl::toPercentEncoding(MetadataParsing::cleanTitle(task.title))));

    get(url, sgdbHeaders(), [this, task, done, steam](int status, const QByteArray &body) {
        if (status == 401 || status == 403) {
            qCWarning(lcMeta) << "SteamGridDB recusou a chave (status" << status << ") - confira metadados/steamgriddb";
            m_config.steamGridDbKey.clear(); // não insiste com uma chave inválida
            done();
            return;
        }
        const QJsonValue data = QJsonDocument::fromJson(body).object().value(QStringLiteral("data"));
        const int sgdbId = steam ? data.toObject().value(QStringLiteral("id")).toInt()
                                 : MetadataParsing::pickSgdbGame(data.toArray(), task.title);
        if (status != 200 && status != 404) {
            m_repository->markArtwork(task.gameId, QStringLiteral("erro"));
            done();
            return;
        }
        if (sgdbId <= 0) {
            qCInfo(lcMeta) << "artes:" << task.title << "não encontrado no SteamGridDB";
            m_repository->markArtwork(task.gameId, QStringLiteral("nao_encontrado"));
            done();
            return;
        }
        fetchSgdbImages(task, sgdbId, done);
    });
}

void MetadataService::fetchSgdbImages(const MetadataTask &task, int sgdbId, Done done)
{
    struct Kind
    {
        bool needed;
        QString type;     // tipo em jogo_midia
        QString endpoint; // grids, heroes, logos
        QString filters;
    };
    const QList<Kind> kinds{
        {task.needSquare, QStringLiteral("quadrada"), QStringLiteral("grids"),
         QStringLiteral("dimensions=1024x1024,512x512&types=static&nsfw=false&humor=false")},
        {task.needHero, QStringLiteral("fundo"), QStringLiteral("heroes"),
         QStringLiteral("types=static&nsfw=false&humor=false")},
        {task.needLogo, QStringLiteral("logo"), QStringLiteral("logos"),
         QStringLiteral("types=static&nsfw=false&humor=false")},
    };

    // Uma cadeia: para cada tipo necessário, lista as imagens, baixa a primeira boa.
    auto pending = std::make_shared<QList<Kind>>();
    for (const Kind &kind : kinds) {
        if (kind.needed)
            pending->append(kind);
    }
    auto saved = std::make_shared<int>(0);
    auto step = std::make_shared<std::function<void()>>();
    *step = [this, task, sgdbId, done, pending, saved, step] {
        if (pending->isEmpty()) {
            m_repository->markArtwork(task.gameId, QStringLiteral("ok"), sgdbId);
            qCInfo(lcMeta) << "artes:" << task.title << *saved << "imagem(ns) baixada(s)";
            done();
            *step = nullptr; // desfaz o ciclo de referência
            return;
        }
        const Kind kind = pending->takeFirst();
        const QUrl url(QStringLiteral("%1/%2/game/%3?%4").arg(SgdbBase, kind.endpoint).arg(sgdbId).arg(kind.filters));
        get(url, sgdbHeaders(), [this, task, kind, saved, step](int status, const QByteArray &body) {
            const QString imageUrl = status == 200
                ? MetadataParsing::pickSgdbImage(QJsonDocument::fromJson(body).object().value(QStringLiteral("data")).toArray())
                : QString();
            if (imageUrl.isEmpty()) {
                (*step)();
                return;
            }
            const QString extension = QFileInfo(QUrl(imageUrl).path()).suffix().toLower();
            const QString path = QStringLiteral("%1/%2/%3.%4").arg(m_config.mediaDir).arg(task.gameId)
                                     .arg(kind.type, extension.isEmpty() ? QStringLiteral("png") : extension);
            download(QUrl(imageUrl), path, [this, task, kind, path, saved, step](bool ok) {
                if (ok) {
                    m_repository->setMedia(task.gameId, kind.type, path, QStringLiteral("steamgriddb"));
                    ++*saved;
                }
                (*step)();
            });
        });
    };
    (*step)();
}

// --- Rede -------------------------------------------------------------------

void MetadataService::get(const QUrl &url, const QList<QPair<QByteArray, QByteArray>> &headers,
                          std::function<void(int, const QByteArray &)> callback)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    for (const auto &[name, value] : headers)
        request.setRawHeader(name, value);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, callback] {
        reply->deleteLater();
        callback(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    });
}

void MetadataService::post(const QUrl &url, const QList<QPair<QByteArray, QByteArray>> &headers,
                           const QByteArray &body, std::function<void(int, const QByteArray &)> callback)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("text/plain"));
    for (const auto &[name, value] : headers)
        request.setRawHeader(name, value);
    QNetworkReply *reply = m_network.post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, callback] {
        reply->deleteLater();
        callback(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    });
}

void MetadataService::download(const QUrl &url, const QString &path, std::function<void(bool)> callback)
{
    get(url, {}, [path, callback](int status, const QByteArray &body) {
        if (status != 200 || body.isEmpty()) {
            callback(false);
            return;
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        const bool ok = file.open(QIODevice::WriteOnly) && file.write(body) == body.size();
        callback(ok);
    });
}
