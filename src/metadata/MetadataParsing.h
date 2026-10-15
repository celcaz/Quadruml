#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QString>

// Funções puras que interpretam as respostas das APIs (testáveis sem internet).
namespace MetadataParsing {

struct GameInfo
{
    bool found = false;
    QString description;
    QString genre;
    QString releaseDate; // como veio da fonte (texto livre ou AAAA-MM-DD)
};

// "UFC® Undisputed™ 3 (USA) [BLUS30738]" -> "UFC Undisputed 3"
QString cleanTitle(const QString &title);

// Igualdade de títulos ignorando maiúsculas, acentos, símbolos e pontuação.
QString normalizedTitle(const QString &title);

// SteamGridDB /search/autocomplete: id do resultado que melhor bate com o título (0 se vazio).
int pickSgdbGame(const QJsonArray &results, const QString &title);

// SteamGridDB /grids, /heroes, /logos: URL da primeira imagem utilizável.
QString pickSgdbImage(const QJsonArray &images);

// Loja da Steam /api/appdetails?appids=<id>
GameInfo parseSteamAppDetails(const QByteArray &json, const QString &appId);

// IGDB /v4/games (resposta de uma busca)
GameInfo parseIgdbGames(const QByteArray &json, const QString &title);

// IGDB: id da plataforma (ps1=7, ps2=8, ps3=9, ps4=48, pc=6), ou 0.
int igdbPlatform(const QString &platform);

// Texto da loja da Steam vem com HTML: tira as tags e troca as entidades comuns.
QString stripHtml(QString text);

}
