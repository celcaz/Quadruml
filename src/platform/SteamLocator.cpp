#include "SteamLocator.h"

#include <QDir>
#include <QSettings>

namespace SteamLocator {

QString steamRoot()
{
#ifdef Q_OS_WIN
    const QSettings user(QStringLiteral("HKEY_CURRENT_USER\\Software\\Valve\\Steam"), QSettings::NativeFormat);
    QString path = user.value(QStringLiteral("SteamPath")).toString();
    if (path.isEmpty()) {
        const QSettings machine(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam"),
                                QSettings::NativeFormat);
        path = machine.value(QStringLiteral("InstallPath")).toString();
    }
#else
    QString path = QDir::homePath() + QStringLiteral("/.steam/steam");
#endif
    if (path.isEmpty() || !QDir(path).exists())
        return {};
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

}
