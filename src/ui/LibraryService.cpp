#include "LibraryService.h"

#include <QJSEngine>

#include "library/GameRepository.h"
#include "library/ScanService.h"

namespace {

LibraryService *s_instance = nullptr;
constexpr int RecentLimit = 12;

// Cor provisória estável para cada jogo (enquanto não há capa).
QColor accentFor(const QString &title)
{
    const int hue = int(qHash(title) % 360);
    return QColor::fromHsl(hue, 90, 70);
}

}

LibraryService::LibraryService(GameRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

LibraryService::~LibraryService()
{
    s_instance = nullptr;
}

LibraryService *LibraryService::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void LibraryService::reload()
{
    const QList<StoredGame> stored = m_repository->recent(RecentLimit);
    if (stored.isEmpty()) {
        loadSampleData();
        return;
    }

    QList<GameEntry> games;
    games.reserve(stored.size());
    for (const StoredGame &game : stored) {
        GameEntry entry;
        entry.id = game.id;
        entry.title = game.title;
        entry.platform = game.platform;
        entry.provider = game.provider;
        entry.accent = accentFor(game.title);
        if (!game.coverPath.isEmpty())
            entry.coverUrl = QUrl::fromLocalFile(game.coverPath);
        if (!game.heroPath.isEmpty())
            entry.heroUrl = QUrl::fromLocalFile(game.heroPath);
        if (!game.logoPath.isEmpty())
            entry.logoUrl = QUrl::fromLocalFile(game.logoPath);

        if (game.identificationPending)
            entry.description = tr("Jogo não identificado pelo código do disco. Dá para corrigir na tela de detalhes (Etapa 8).");
        else if (game.provider == QLatin1String("steam"))
            entry.description = tr("Steam · App %1. A descrição chega na Etapa 7.").arg(game.externalId);
        else
            entry.description = tr("Código do disco: %1. Descrição e artes chegam na Etapa 7.").arg(game.externalId);
        games.append(entry);
    }
    m_recent.setGames(std::move(games));
    m_usingSampleData = false;
    emit changed();
}

void LibraryService::setScanning(bool scanning)
{
    if (scanning == m_scanning)
        return;
    m_scanning = scanning;
    emit scanningChanged();
}

void LibraryService::setScanResult(const ScanSummary &summary)
{
    QStringList platforms;
    for (auto it = summary.byPlatform.cbegin(); it != summary.byPlatform.cend(); ++it)
        platforms.append(QStringLiteral("%1: %2").arg(GameListModel::platformLabel(it.key())).arg(it.value()));
    m_scanSummary = tr("%1 · %2 pendente(s) · varredura em %3 ms")
                        .arg(platforms.isEmpty() ? tr("nenhum jogo") : platforms.join(QStringLiteral(", ")))
                        .arg(summary.pending)
                        .arg(summary.elapsedMs);
    setScanning(false);
    reload();
}

void LibraryService::loadSampleData()
{
    // Dados de exemplo só para a tela não ficar vazia antes da primeira varredura.
    auto game = [](qint64 id, const char *title, const char *platform, const char *color, int minutes) {
        GameEntry entry;
        entry.id = id;
        entry.title = QString::fromUtf8(title);
        entry.platform = QString::fromLatin1(platform);
        entry.accent = QColor(QString::fromLatin1(color));
        entry.minutesPlayed = minutes;
        entry.description = tr("Jogo de exemplo. Configure as pastas de jogos para ver os seus.");
        return entry;
    };

    m_recent.setGames({
        game(1, "Shadow of the Colossus", "ps2", "#6b7f8f", 2040),
        game(2, "Bloodborne", "ps4", "#5a2a2a", 5100),
        game(3, "Final Fantasy VII", "ps1", "#2f4f7f", 3300),
        game(4, "Hollow Knight", "pc", "#3a4a6a", 1800),
        game(5, "Demon's Souls", "ps3", "#4a4a3a", 2700),
        game(6, "God of War", "ps2", "#7f2f1f", 960),
    });
    m_usingSampleData = true;
    emit changed();
}
