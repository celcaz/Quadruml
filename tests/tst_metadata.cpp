#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "metadata/MetadataParsing.h"

using namespace MetadataParsing;

class TestMetadata : public QObject
{
    Q_OBJECT

private slots:
    void cleansTitles()
    {
        QCOMPARE(cleanTitle(QStringLiteral("UFC® Undisputed™ 3")), QStringLiteral("UFC Undisputed 3"));
        QCOMPARE(cleanTitle(QStringLiteral("Shadow of Rome (USA) [SLUS-20902]")), QStringLiteral("Shadow of Rome"));
        QCOMPARE(normalizedTitle(QStringLiteral("skate.")), normalizedTitle(QStringLiteral("Skate")));
        QCOMPARE(normalizedTitle(QStringLiteral("Dante's Inferno™")), QStringLiteral("dantesinferno"));
    }

    void picksExactSgdbMatchFirst()
    {
        const QJsonArray results = QJsonDocument::fromJson(
            "[{\"id\": 1, \"name\": \"Skate 3\"}, {\"id\": 2, \"name\": \"skate.\"}, {\"id\": 3, \"name\": \"Skate 2\"}]").array();
        QCOMPARE(pickSgdbGame(results, QStringLiteral("skate.")), 2);
        QCOMPARE(pickSgdbGame(results, QStringLiteral("Algo diferente")), 1); // mais relevante
        QCOMPARE(pickSgdbGame(QJsonArray(), QStringLiteral("x")), 0);
    }

    void skipsNsfwAndHumorImages()
    {
        const QJsonArray images = QJsonDocument::fromJson(
            "[{\"url\": \"https://a/1.png\", \"nsfw\": true, \"humor\": false},"
            " {\"url\": \"https://a/2.png\", \"nsfw\": false, \"humor\": true},"
            " {\"url\": \"https://a/3.png\", \"nsfw\": false, \"humor\": false}]").array();
        QCOMPARE(pickSgdbImage(images), QStringLiteral("https://a/3.png"));
    }

    void parsesSteamAppDetails()
    {
        const QByteArray json =
            "{\"952060\": {\"success\": true, \"data\": {"
            "\"short_description\": \"Jill Valentine &quot;volta&quot; <b>em</b> Raccoon City.\","
            "\"genres\": [{\"id\": \"1\", \"description\": \"A\xc3\xa7\xc3\xa3o\"}, {\"id\": \"25\", \"description\": \"Aventura\"},"
            " {\"id\": \"9\", \"description\": \"Terror\"}],"
            "\"release_date\": {\"coming_soon\": false, \"date\": \"2 abr. 2020\"}}}}";
        const GameInfo info = parseSteamAppDetails(json, QStringLiteral("952060"));
        QVERIFY(info.found);
        QCOMPARE(info.description, QStringLiteral("Jill Valentine \"volta\" em Raccoon City."));
        QCOMPARE(info.genre, QStringLiteral("Ação, Aventura"));
        QCOMPARE(info.releaseDate, QStringLiteral("2 abr. 2020"));

        QVERIFY(!parseSteamAppDetails("{\"1\": {\"success\": false}}", QStringLiteral("1")).found);
    }

    void parsesIgdbGames()
    {
        const QByteArray json =
            "[{\"id\": 1, \"name\": \"Shadow of Rome 2\", \"summary\": \"Outro.\"},"
            " {\"id\": 2, \"name\": \"Shadow of Rome\", \"summary\": \"Gladiadores em Roma.\","
            " \"first_release_date\": 1108425600, \"genres\": [{\"name\": \"Hack and slash\"}]}]";
        const GameInfo info = parseIgdbGames(json, QStringLiteral("Shadow of Rome"));
        QVERIFY(info.found);
        QCOMPARE(info.description, QStringLiteral("Gladiadores em Roma."));
        QCOMPARE(info.genre, QStringLiteral("Hack and slash"));
        QCOMPARE(info.releaseDate, QStringLiteral("2005-02-15"));
    }

    void mapsIgdbPlatforms()
    {
        QCOMPARE(igdbPlatform(QStringLiteral("ps2")), 8);
        QCOMPARE(igdbPlatform(QStringLiteral("ps3")), 9);
        QCOMPARE(igdbPlatform(QStringLiteral("xbox")), 0);
    }
};

QTEST_GUILESS_MAIN(TestMetadata)
#include "tst_metadata.moc"
