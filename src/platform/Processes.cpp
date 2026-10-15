#include "Processes.h"

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace Processes {

bool isRunning(const QString &executableName)
{
#ifdef Q_OS_WIN
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    for (BOOL ok = Process32FirstW(snapshot, &entry); ok && !found; ok = Process32NextW(snapshot, &entry))
        found = QString::fromWCharArray(entry.szExeFile).compare(executableName, Qt::CaseInsensitive) == 0;

    CloseHandle(snapshot);
    return found;
#else
    Q_UNUSED(executableName);
    return false;
#endif
}

}
