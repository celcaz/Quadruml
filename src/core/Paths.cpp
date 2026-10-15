#include "Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include "AppInfo.h"

namespace Paths {

QString dataRoot()
{
    // No Windows: %LOCALAPPDATA%/QuadrumI
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QString configFile()
{
    return dataRoot() + QStringLiteral("/config.ini");
}

QString databaseFile()
{
    return dataRoot() + QStringLiteral("/dados/hub.db");
}

QString logsDir()
{
    return dataRoot() + QStringLiteral("/logs");
}

QString mediaDir()
{
    return dataRoot() + QStringLiteral("/midia");
}

QStringList themeSearchDirs()
{
    QStringList dirs{QCoreApplication::applicationDirPath() + QStringLiteral("/themes")};
    const QString sourceThemes = QString::fromUtf8(AppInfo::SourceThemesDir);
    if (QDir(sourceThemes).exists())
        dirs.append(sourceThemes);
    return dirs;
}

void ensureCreated()
{
    QDir dir;
    for (const QString &path : {dataRoot(), dataRoot() + QStringLiteral("/dados"), logsDir(), mediaDir()})
        dir.mkpath(path);
}

}
