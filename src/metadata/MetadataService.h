#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include <functional>

#include "library/GameRepository.h"

struct MetadataConfig
{
    QString steamGridDbKey;
    QString twitchClientId;
    QString twitchClientSecret;
    QString mediaDir;
};

// Busca, em segundo plano, o que falta para cada jogo:
//  - artes no SteamGridDB (capa quadrada, fundo, logo), baixadas para a pasta de mídia;
//  - descrição: loja da Steam (jogos da Steam, em português) ou IGDB (consoles).
// Um jogo por vez, com intervalo entre pedidos, para respeitar os serviços.
class MetadataService : public QObject
{
    Q_OBJECT

public:
    MetadataService(GameRepository *repository, MetadataConfig config, QObject *parent = nullptr);

    void start();
    bool running() const { return m_running; }

signals:
    void gameUpdated(qint64 gameId);
    void finished(int processed);

private:
    using Done = std::function<void()>;
    using Step = std::function<void(Done)>;

    void nextTask();
    void runSteps();

    // Etapas de um jogo
    void steamInfo(const MetadataTask &task, Done done);
    void igdbInfo(const MetadataTask &task, Done done);
    void artwork(const MetadataTask &task, Done done);
    void fetchSgdbImages(const MetadataTask &task, int sgdbId, Done done);

    // Rede
    void get(const QUrl &url, const QList<QPair<QByteArray, QByteArray>> &headers,
             std::function<void(int status, const QByteArray &body)> callback);
    void post(const QUrl &url, const QList<QPair<QByteArray, QByteArray>> &headers, const QByteArray &body,
              std::function<void(int status, const QByteArray &body)> callback);
    void download(const QUrl &url, const QString &path, std::function<void(bool ok)> callback);
    void withIgdbToken(std::function<void(bool ok)> callback);

    QList<QPair<QByteArray, QByteArray>> sgdbHeaders() const;

    GameRepository *m_repository;
    MetadataConfig m_config;
    QNetworkAccessManager m_network;
    QList<MetadataTask> m_tasks;
    QList<Step> m_steps;
    QString m_igdbToken;
    bool m_running = false;
    int m_processed = 0;
};
