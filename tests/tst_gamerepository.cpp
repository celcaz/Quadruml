#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "core/Database.h"
#include "library/GameRepository.h"

class TestGameRepository : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    std::unique_ptr<Database> m_db;
    int m_counter = 0;

    ScannedGame game(const QString &id, const QString &title, const QStringList &paths)
    {
        ScannedGame g;
        g.platform = QStringLiteral("ps2");
        g.provider = QStringLiteral("ps2");
        g.externalId = id;
        g.title = title;
        for (const QString &path : paths) {
            ScannedFile file;
            file.path = path;
            file.format = QStringLiteral("iso");
            file.size = 100;
            file.modified = QDateTime(QDate(2026, 1, 1), QTime(12, 0), QTimeZone::UTC);
            g.files.append(file);
        }
        return g;
    }

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(m_dir.filePath(QStringLiteral("hub%1.db").arg(++m_counter)),
                                          QStringLiteral("repo"));
        QVERIFY(m_db->open());
    }

    void cleanup() { m_db.reset(); }

    void insertsAndUpdatesByExternalId()
    {
        GameRepository repo(m_db->handle());
        const qint64 a = repo.upsert(game(QStringLiteral("SLUS-20946"), QStringLiteral("Shadow of Rome"), {QStringLiteral("D:/a.iso")}));
        const qint64 b = repo.upsert(game(QStringLiteral("SLUS-20946"), QStringLiteral("Shadow of Rome (USA)"), {QStringLiteral("D:/a.iso")}));
        QCOMPARE(a, b);
        QCOMPARE(repo.recent(10).size(), 1);
        QCOMPARE(repo.recent(10).first().title, QStringLiteral("Shadow of Rome (USA)"));
        QCOMPARE(repo.countByPlatform().value(QStringLiteral("ps2")), 1);
    }

    void detectsUnchangedFiles()
    {
        GameRepository repo(m_db->handle());
        const ScannedGame g = game(QStringLiteral("SLUS-1"), QStringLiteral("A"), {QStringLiteral("D:/a.iso")});
        repo.upsert(g);
        const ScannedFile &file = g.files.first();
        QVERIFY(repo.isUnchanged(file.path, file.size, file.modified));
        QVERIFY(!repo.isUnchanged(file.path, file.size + 1, file.modified));
        QVERIFY(!repo.isUnchanged(QStringLiteral("D:/outro.iso"), file.size, file.modified));
    }

    void removesMissingFilesOnlyUnderExistingRoots()
    {
        GameRepository repo(m_db->handle());
        const QString root = QDir::fromNativeSeparators(m_dir.path()) + QStringLiteral("/jogos");
        QVERIFY(QDir().mkpath(root));

        repo.upsert(game(QStringLiteral("SLUS-1"), QStringLiteral("Fica"), {root + QStringLiteral("/fica.iso")}));
        repo.upsert(game(QStringLiteral("SLUS-2"), QStringLiteral("Sai"), {root + QStringLiteral("/sai.iso")}));
        repo.upsert(game(QStringLiteral("SLUS-3"), QStringLiteral("Disco desligado"), {QStringLiteral("Z:/naoexiste/x.iso")}));

        const int removed = repo.removeMissing(QStringLiteral("ps2"),
                                               {root, QStringLiteral("Z:/naoexiste")},
                                               {root + QStringLiteral("/fica.iso")});
        QCOMPARE(removed, 1);

        QStringList titles;
        for (const StoredGame &g : repo.recent(10))
            titles.append(g.title);
        titles.sort();
        QCOMPARE(titles, (QStringList{QStringLiteral("Disco desligado"), QStringLiteral("Fica")}));
    }

    void sortTitleIgnoresCaseAndAccents()
    {
        QCOMPARE(GameRepository::sortTitle(QStringLiteral("Ébano Ação")), QStringLiteral("ebano acao"));
    }
};

QTEST_GUILESS_MAIN(TestGameRepository)
#include "tst_gamerepository.moc"
