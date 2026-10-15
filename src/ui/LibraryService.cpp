#include "LibraryService.h"

#include <QJSEngine>

namespace {
LibraryService *s_instance = nullptr;
}

LibraryService::LibraryService(QObject *parent)
    : QObject(parent)
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

void LibraryService::loadSampleData()
{
    // Dados de exemplo só para desenhar as telas. Saem na Etapa 7.
    auto game = [](qint64 id, const char *title, const char *platform, const char *color, int minutes,
                   const char *description) {
        GameEntry entry;
        entry.id = id;
        entry.title = QString::fromUtf8(title);
        entry.platform = QString::fromLatin1(platform);
        entry.accent = QColor(QString::fromLatin1(color));
        entry.minutesPlayed = minutes;
        entry.description = QString::fromUtf8(description);
        return entry;
    };

    m_recent.setGames({
        game(1, "Shadow of the Colossus", "ps2", "#6b7f8f", 2040, "Jogo de exemplo para desenhar a tela inicial."),
        game(2, "Bloodborne", "ps4", "#5a2a2a", 5100, "Jogo de exemplo para desenhar a tela inicial."),
        game(3, "Final Fantasy VII", "ps1", "#2f4f7f", 3300, "Jogo de exemplo para desenhar a tela inicial."),
        game(4, "Hollow Knight", "pc", "#3a4a6a", 1800, "Jogo de exemplo para desenhar a tela inicial."),
        game(5, "Demon's Souls", "ps3", "#4a4a3a", 2700, "Jogo de exemplo para desenhar a tela inicial."),
        game(6, "God of War", "ps2", "#7f2f1f", 960, "Jogo de exemplo para desenhar a tela inicial."),
        game(7, "Metal Gear Solid", "ps1", "#2f5f4f", 720, "Jogo de exemplo para desenhar a tela inicial."),
        game(8, "Elden Ring", "pc", "#8f7a3f", 6000, "Jogo de exemplo para desenhar a tela inicial."),
        game(9, "The Last of Us", "ps3", "#3f5a3a", 1500, "Jogo de exemplo para desenhar a tela inicial."),
    });
    m_usingSampleData = true;
    emit changed();
}
