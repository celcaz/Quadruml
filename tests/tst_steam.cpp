#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "core/Database.h"
#include "library/GameRepository.h"
#include "library/Providers.h"
#include "library/Vdf.h"

namespace {

void writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

QByteArray manifest(const char *appId, const char *name, int flags)
{
    return QByteArray("\"AppState\"\n{\n\t\"appid\"\t\t\"") + appId + "\"\n\t\"name\"\t\t\"" + name
           + "\"\n\t\"StateFlags\"\t\t\"" + QByteArray::number(flags) + "\"\n}\n";
}

}

class TestSteam : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_steam;
    QString m_extraLibrary;

private slots:
    void initTestCase()
    {
        m_steam = QDir::fromNativeSeparators(m_dir.path()) + QStringLiteral("/Steam");
        m_extraLibrary = QDir::fromNativeSeparators(m_dir.path()) + QStringLiteral("/Biblioteca2");

        // Duas bibliotecas; caminhos com barras invertidas escapadas, como no arquivo real.
        QString extraEscaped = QDir::toNativeSeparators(m_extraLibrary);
        extraEscaped.replace(QLatin1String("\\"), QLatin1String("\\\\"));
        writeFile(m_steam + QStringLiteral("/steamapps/libraryfolders.vdf"),
                  "\"libraryfolders\"\n{\n\t\"0\"\n\t{\n\t\t\"path\"\t\t\"" + QDir::toNativeSeparators(m_steam).replace(QLatin1String("\\"), QLatin1String("\\\\")).toUtf8()
                      + "\"\n\t}\n\t\"1\"\n\t{\n\t\t\"path\"\t\t\"" + extraEscaped.toUtf8() + "\"\n\t}\n}\n");

        writeFile(m_steam + QStringLiteral("/steamapps/appmanifest_1174180.acf"),
                  manifest("1174180", "Red Dead Redemption 2", 4));
        writeFile(m_steam + QStringLiteral("/steamapps/appmanifest_228980.acf"),
                  manifest("228980", "Steamworks Common Redistributables", 4));
        writeFile(m_steam + QStringLiteral("/steamapps/appmanifest_999.acf"),
                  manifest("999", "Baixando", 1026)); // sem o bit 4: não instalado
        writeFile(m_extraLibrary + QStringLiteral("/steamapps/appmanifest_1938090.acf"),
                  manifest("1938090", "Call of Duty\xc2\xae", 22)); // bit 4 + atualização pendente

        // Cache antigo (arquivos direto na pasta) e novo (subpastas).
        const QString cache = m_steam + QStringLiteral("/appcache/librarycache/");
        writeFile(cache + QStringLiteral("1174180/library_600x900.jpg"), "x");
        writeFile(cache + QStringLiteral("1174180/library_hero.jpg"), "x");
        writeFile(cache + QStringLiteral("1174180/logo.png"), "x");
        writeFile(cache + QStringLiteral("1938090/abc123/library_capsule.jpg"), "x");
        writeFile(cache + QStringLiteral("1938090/def456/library_hero.jpg"), "x");
    }

    void parsesVdf()
    {
        const QVariantMap root = Vdf::parse(
            "// comentário\n\"a\"\n{\n\t\"b\"\t\"1\"\n\t\"c\"\n\t{\n\t\t\"d\" \"C:\\\\Jogos\\\\X\"\n\t}\n}\n");
        const QVariantMap a = root.value(QStringLiteral("a")).toMap();
        QCOMPARE(a.value(QStringLiteral("b")).toString(), QStringLiteral("1"));
        QCOMPARE(a.value(QStringLiteral("c")).toMap().value(QStringLiteral("d")).toString(),
                 QStringLiteral("C:\\Jogos\\X"));
    }

    void findsBothLibraries()
    {
        const QStringList folders = SteamProvider::libraryFolders(m_steam);
        QCOMPARE(folders.size(), 2);
        QVERIFY(folders.contains(m_extraLibrary + QStringLiteral("/steamapps"), Qt::CaseInsensitive));
    }

    void findsArtworkInBothCacheFormats()
    {
        const auto oldFormat = SteamProvider::localArtwork(m_steam, QStringLiteral("1174180"));
        QCOMPARE(oldFormat.keys(), (QStringList{QStringLiteral("capa"), QStringLiteral("fundo"), QStringLiteral("logo")}));

        const auto newFormat = SteamProvider::localArtwork(m_steam, QStringLiteral("1938090"));
        QVERIFY(newFormat.value(QStringLiteral("capa")).endsWith(QLatin1String("library_capsule.jpg")));
        QVERIFY(newFormat.contains(QStringLiteral("fundo")));
        QVERIFY(!newFormat.contains(QStringLiteral("logo")));
    }

    void scansInstalledGamesOnly()
    {
        Database db(m_dir.filePath(QStringLiteral("hub.db")), QStringLiteral("steam"));
        QVERIFY(db.open());
        GameRepository repository(db.handle());

        const ProviderScan scan = SteamProvider().scan(m_steam, repository);
        QStringList titles;
        for (const ScannedGame &game : scan.changed)
            titles.append(game.title);
        titles.sort();
        QCOMPARE(titles, (QStringList{QStringLiteral("Call of Duty®"), QStringLiteral("Red Dead Redemption 2")}));

        for (const ScannedGame &game : scan.changed)
            repository.upsert(game);
        const QList<StoredGame> stored = repository.recent(10);
        QCOMPARE(stored.size(), 2);
        for (const StoredGame &game : stored) {
            QCOMPARE(game.platform, QStringLiteral("pc"));
            QCOMPARE(game.provider, QStringLiteral("steam"));
            QVERIFY(!game.coverPath.isEmpty());
        }

        // Segunda varredura: nada mudou.
        const ProviderScan again = SteamProvider().scan(m_steam, repository);
        QCOMPARE(again.changed.size(), 0);
        QCOMPARE(again.unchanged, 2);
    }

    void identifiesTools()
    {
        QVERIFY(SteamProvider::isTool(QStringLiteral("228980"), QStringLiteral("x")));
        QVERIFY(SteamProvider::isTool(QStringLiteral("1"), QStringLiteral("Proton 9.0")));
        QVERIFY(!SteamProvider::isTool(QStringLiteral("952060"), QStringLiteral("Resident Evil 3")));
    }
};

QTEST_GUILESS_MAIN(TestSteam)
#include "tst_steam.moc"
