#include <QSignalSpy>
#include <QTest>

#include "GameListModel.h"

namespace {

GameEntry game(qint64 id, const QString &title)
{
    GameEntry entry;
    entry.id = id;
    entry.title = title;
    entry.platform = QStringLiteral("ps2");
    return entry;
}

}

class TestGameListModel : public QObject
{
    Q_OBJECT

private slots:
    void sameDataChangesNothing()
    {
        GameListModel model(true);
        model.setGames({game(1, QStringLiteral("A")), game(2, QStringLiteral("B"))});

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        QVERIFY(!model.setGames({game(1, QStringLiteral("A")), game(2, QStringLiteral("B"))}));
        QCOMPARE(reset.count(), 0);
        QCOMPARE(changed.count(), 0);
    }

    void newCoverUpdatesOnlyThatRow()
    {
        GameListModel model(true);
        model.setGames({game(1, QStringLiteral("A")), game(2, QStringLiteral("B")), game(3, QStringLiteral("C"))});

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        GameEntry withCover = game(2, QStringLiteral("B"));
        withCover.coverUrl = QUrl::fromLocalFile(QStringLiteral("C:/capa.png"));
        QVERIFY(model.setGames({game(1, QStringLiteral("A")), withCover, game(3, QStringLiteral("C"))}));

        QCOMPARE(reset.count(), 0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().at(0).toModelIndex().row(), 1);
        QCOMPARE(model.data(model.index(1), GameListModel::CoverUrlRole).toUrl(), withCover.coverUrl);
    }

    void newGameRebuildsList()
    {
        GameListModel model(true);
        model.setGames({game(1, QStringLiteral("A"))});

        QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
        QVERIFY(model.setGames({game(2, QStringLiteral("Novo")), game(1, QStringLiteral("A"))}));
        QCOMPARE(reset.count(), 1);
        QCOMPARE(model.rowCount(), 3); // 2 jogos + item "Biblioteca"
    }
};

QTEST_GUILESS_MAIN(TestGameListModel)
#include "tst_gamelistmodel.moc"
