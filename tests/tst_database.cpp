#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include "core/Database.h"

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void createsSchemaOnFirstOpen()
    {
        QTemporaryDir dir;
        Database db(dir.filePath(QStringLiteral("hub.db")), QStringLiteral("t1"));
        QVERIFY2(db.open(), qPrintable(db.lastError()));
        QCOMPARE(db.schemaVersion(), Database::latestVersion());

        QSqlQuery query(db.handle());
        for (const char *table : {"perfis", "jogos", "jogo_arquivos", "jogo_midia", "jogo_info", "pastas", "config"}) {
            query.prepare(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?"));
            query.addBindValue(QString::fromLatin1(table));
            QVERIFY(query.exec());
            QVERIFY2(query.next(), table);
        }
    }

    void reopeningKeepsDataAndVersion()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("hub.db"));
        {
            Database db(path, QStringLiteral("t2a"));
            QVERIFY(db.open());
            QSqlQuery query(db.handle());
            QVERIFY(query.exec(QStringLiteral("INSERT INTO config (chave, valor) VALUES ('x', '1')")));
        }
        Database db(path, QStringLiteral("t2b"));
        QVERIFY(db.open());
        QCOMPARE(db.schemaVersion(), Database::latestVersion());
        QSqlQuery query(db.handle());
        QVERIFY(query.exec(QStringLiteral("SELECT valor FROM config WHERE chave = 'x'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("1"));
    }

    void foreignKeysCascade()
    {
        QTemporaryDir dir;
        Database db(dir.filePath(QStringLiteral("hub.db")), QStringLiteral("t3"));
        QVERIFY(db.open());
        QSqlQuery query(db.handle());
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO jogos (plataforma, provedor, id_externo, titulo, titulo_ordenacao)"
            " VALUES ('ps2', 'ps2', 'SLUS-20946', 'Jogo', 'jogo')")));
        const qint64 gameId = query.lastInsertId().toLongLong();
        query.prepare(QStringLiteral("INSERT INTO jogo_arquivos (jogo_id, caminho) VALUES (?, 'C:/jogo.iso')"));
        query.addBindValue(gameId);
        QVERIFY(query.exec());

        QVERIFY(query.exec(QStringLiteral("DELETE FROM jogos")));
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM jogo_arquivos")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }
};

QTEST_GUILESS_MAIN(TestDatabase)
#include "tst_database.moc"
