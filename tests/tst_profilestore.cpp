#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "core/Database.h"
#include "core/ProfileStore.h"

class TestProfileStore : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    std::unique_ptr<Database> m_db;

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(m_dir.filePath(QStringLiteral("hub.db")), QStringLiteral("profiles"));
        QVERIFY(m_db->open());
        QSqlQuery(m_db->handle()).exec(QStringLiteral("DELETE FROM perfis"));
    }

    void cleanup() { m_db.reset(); }

    void createsAndLists()
    {
        ProfileStore store(m_db->handle());
        const qint64 a = store.create(QStringLiteral("  Celio  "));
        const qint64 b = store.create(QStringLiteral("Visita"));
        QVERIFY(a > 0 && b > 0);

        const QList<Profile> all = store.all();
        QCOMPARE(all.size(), 2);
        QCOMPARE(all.at(0).name, QStringLiteral("Celio"));
        QVERIFY(!all.at(0).hasPin);
    }

    void rejectsEmptyName()
    {
        ProfileStore store(m_db->handle());
        QCOMPARE(store.create(QStringLiteral("   ")), 0);
        QVERIFY(!store.rename(store.create(QStringLiteral("A")), QString()));
    }

    void truncatesLongName()
    {
        ProfileStore store(m_db->handle());
        const qint64 id = store.create(QString(50, QLatin1Char('x')));
        QCOMPARE(store.find(id)->name.size(), ProfileStore::MaxNameLength);
    }

    void pinRoundTrip()
    {
        ProfileStore store(m_db->handle());
        const qint64 id = store.create(QStringLiteral("Celio"));

        QVERIFY(store.checkPin(id, QString())); // sem PIN, qualquer entrada passa
        QVERIFY(store.setPin(id, QStringLiteral("1234")));
        QVERIFY(store.find(id)->hasPin);
        QVERIFY(store.checkPin(id, QStringLiteral("1234")));
        QVERIFY(!store.checkPin(id, QStringLiteral("4321")));
        QVERIFY(!store.checkPin(id, QString()));

        QVERIFY(store.clearPin(id));
        QVERIFY(!store.find(id)->hasPin);
    }

    void pinIsNeverStoredInPlainText()
    {
        ProfileStore store(m_db->handle());
        const qint64 id = store.create(QStringLiteral("Celio"));
        QVERIFY(store.setPin(id, QStringLiteral("2468")));

        QSqlQuery query(m_db->handle());
        QVERIFY(query.exec(QStringLiteral("SELECT pin_hash FROM perfis")));
        QVERIFY(query.next());
        QVERIFY(!query.value(0).toString().contains(QStringLiteral("2468")));
    }

    void validatesPinFormat()
    {
        QVERIFY(ProfileStore::isValidPin(QStringLiteral("0000")));
        QVERIFY(!ProfileStore::isValidPin(QStringLiteral("123")));
        QVERIFY(!ProfileStore::isValidPin(QStringLiteral("12345")));
        QVERIFY(!ProfileStore::isValidPin(QStringLiteral("12a4")));
    }

    void removes()
    {
        ProfileStore store(m_db->handle());
        const qint64 id = store.create(QStringLiteral("Temp"));
        QVERIFY(store.remove(id));
        QVERIFY(!store.find(id));
        QVERIFY(!store.remove(id));
    }
};

QTEST_GUILESS_MAIN(TestProfileStore)
#include "tst_profilestore.moc"
