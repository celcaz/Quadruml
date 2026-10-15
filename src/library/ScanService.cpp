#include "ScanService.h"

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QThread>

#include "GameRepository.h"
#include "Providers.h"
#include "core/Database.h"
#include "titles/Ps2TitleDb.h"

Q_DECLARE_LOGGING_CATEGORY(lcScan)

namespace ScanService {

ScanSummary run(const QString &databasePath, const ScanConfig &config)
{
    QElapsedTimer timer;
    timer.start();
    ScanSummary summary;

    // Conexão própria desta thread (o SQLite em modo WAL aceita leitura e escrita juntas).
    const QString connection = QStringLiteral("scan-%1").arg(quintptr(QThread::currentThreadId()));
    Database database(databasePath, connection);
    if (!database.open())
        return summary;
    GameRepository repository(database.handle());

    auto apply = [&](const char *providerId, const QStringList &folders, const ProviderScan &scan) {
        for (const ScannedGame &game : scan.changed)
            repository.upsert(game);
        summary.changed += int(scan.changed.size());
        summary.unchanged += scan.unchanged;
        summary.removed += repository.removeMissing(QString::fromLatin1(providerId), folders, scan.seenPaths);
    };

    if (!config.ps2Folders.isEmpty()) {
        Ps2Provider ps2(config.pcsx2Dir + QStringLiteral("/resources/GameIndex.yaml"));
        apply(Ps2Provider::Id, config.ps2Folders, ps2.scan(config.ps2Folders, repository));
    }

    if (!config.ps3Folders.isEmpty())
        apply(Ps3Provider::Id, config.ps3Folders, Ps3Provider().scan(config.ps3Folders, repository));

    summary.byPlatform = repository.countByPlatform();
    summary.pending = repository.pendingCount();
    summary.elapsedMs = timer.elapsed();
    qCInfo(lcScan) << "varredura concluída em" << summary.elapsedMs << "ms:" << summary.changed << "novos/atualizados,"
                   << summary.unchanged << "sem mudança," << summary.removed << "removidos," << summary.pending
                   << "pendentes; por plataforma:" << summary.byPlatform;
    return summary;
}

}
