#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QtConcurrent/QtConcurrentRun>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

#include "AppInfo.h"
#include "core/Database.h"
#include "core/Logger.h"
#include "core/Paths.h"
#include "core/ProfileStore.h"
#include "input/GamepadService.h"
#include "library/GameRepository.h"
#include "library/ScanService.h"
#include "platform/SteamLocator.h"
#include "platform/Windowing.h"
#include "ui/LanguageService.h"
#include "ui/LibraryService.h"
#include "ui/ProfileService.h"
#include "ui/ThemeService.h"

Q_LOGGING_CATEGORY(lcApp, "quadrum.app")

int main(int argc, char *argv[])
{
    QElapsedTimer startup;
    startup.start();

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(AppInfo::Id);
    QGuiApplication::setApplicationDisplayName(AppInfo::DisplayName);
    QGuiApplication::setApplicationVersion(AppInfo::Version);

    Paths::ensureCreated();
    Logger::install(Paths::logsDir());
    qCInfo(lcApp) << AppInfo::DisplayName << AppInfo::Version << "iniciando; dados em" << Paths::dataRoot();

    QSettings settings(Paths::configFile(), QSettings::IniFormat);

    Database database(Paths::databaseFile());
    if (!database.open())
        return 1;
    ProfileStore profileStore(database.handle());

    // Os serviços são criados antes do engine para serem destruídos depois dele.
    ThemeService theme(nullptr);
    theme.load(settings.value("tema", "ps5").toString());

    ProfileService profiles(&profileStore, nullptr);

    GameRepository gameRepository(database.handle());
    LibraryService library(&gameRepository, nullptr);
    library.reload();

    // Pastas de jogos e emuladores. Até a tela de Configurações (Etapa 8), vêm do config.ini:
    //   [pastas] ps2=D:/GAMES/PS2/GAMES   [emuladores] pcsx2=D:/GAMES/PS2/PCSX2
    ScanConfig scanConfig;
    scanConfig.ps2Folders = settings.value("pastas/ps2").toStringList();
    scanConfig.ps3Folders = settings.value("pastas/ps3").toStringList();
    scanConfig.pcsx2Dir = settings.value("emuladores/pcsx2").toString();
    // Steam: encontrada sozinha pelo Registro; "lojas/steam=false" desliga.
    if (settings.value("lojas/steam", true).toBool())
        scanConfig.steamRoot = SteamLocator::steamRoot();

    QFutureWatcher<ScanSummary> scanWatcher;
    QObject::connect(&scanWatcher, &QFutureWatcher<ScanSummary>::finished, &library,
                     [&library, &scanWatcher] { library.setScanResult(scanWatcher.result()); });
    auto startScan = [&library, &scanWatcher, scanConfig] {
        library.setScanning(true);
        scanWatcher.setFuture(QtConcurrent::run(&ScanService::run, Paths::databaseFile(), scanConfig));
    };

    // O SDL leva ~150 ms para iniciar: só liga depois do primeiro quadro (abaixo).
    GamepadService gamepad(nullptr);

    LanguageService language(nullptr);
    QObject::connect(&language, &LanguageService::languageChanged, &app, [&settings, &language] {
        settings.setValue("idioma", language.language());
    });

    QQmlApplicationEngine engine;
    language.setEngine(&engine);
    language.setLanguage(settings.value("idioma", LanguageService::sourceLanguage()).toString());
    engine.rootContext()->setContextProperty(QStringLiteral("databaseVersion"), database.schemaVersion());

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Quadrum", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
        QObject::connect(window, &QQuickWindow::frameSwapped, window, [&startup, &gamepad, window, startScan] {
            qCInfo(lcApp) << "primeiro quadro em" << startup.elapsed() << "ms";
            Windowing::bringToFront(window);
            gamepad.start();
            startScan();
        }, Qt::SingleShotConnection);

        // Desenvolvimento: QUADRUM_SCREENSHOT=caminho.png salva uma captura da janela
        // depois de alguns segundos (para conferir o visual sem olhar a tela).
        // QUADRUM_DEV_KEYS=Right,Right,Down simula teclas antes da captura.
        const QStringList devKeys = qEnvironmentVariable("QUADRUM_DEV_KEYS").split(u',', Qt::SkipEmptyParts);
        const QHash<QString, int> keyNames{
            {QStringLiteral("Left"), Qt::Key_Left}, {QStringLiteral("Right"), Qt::Key_Right},
            {QStringLiteral("Up"), Qt::Key_Up}, {QStringLiteral("Down"), Qt::Key_Down},
            {QStringLiteral("Return"), Qt::Key_Return}, {QStringLiteral("Escape"), Qt::Key_Escape},
        };
        for (int i = 0; i < devKeys.size(); ++i) {
            const int key = keyNames.value(devKeys.at(i).trimmed(), 0);
            QTimer::singleShot(1500 + i * 400, window, [window, key] {
                QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
                QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &release);
            });
        }

        const QString screenshot = qEnvironmentVariable("QUADRUM_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            QTimer::singleShot(3000 + int(devKeys.size()) * 400, window, [window, screenshot] {
                const bool saved = window->grabWindow().save(screenshot);
                qCInfo(lcApp) << "captura" << (saved ? "salva em" : "falhou:") << screenshot;
            });
        }
    }

    return app.exec();
}
