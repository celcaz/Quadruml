#include <QElapsedTimer>
#include <QGuiApplication>
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

    LibraryService library(nullptr);
    library.loadSampleData(); // até a Etapa 7 (biblioteca real)

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
        QObject::connect(window, &QQuickWindow::frameSwapped, window, [&startup, &gamepad] {
            qCInfo(lcApp) << "primeiro quadro em" << startup.elapsed() << "ms";
            gamepad.start();
        }, Qt::SingleShotConnection);

        // Desenvolvimento: QUADRUM_SCREENSHOT=caminho.png salva uma captura da janela
        // depois de alguns segundos (para conferir o visual sem olhar a tela).
        const QString screenshot = qEnvironmentVariable("QUADRUM_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            QTimer::singleShot(3000, window, [window, screenshot] {
                const bool saved = window->grabWindow().save(screenshot);
                qCInfo(lcApp) << "captura" << (saved ? "salva em" : "falhou:") << screenshot;
            });
        }
    }

    return app.exec();
}
