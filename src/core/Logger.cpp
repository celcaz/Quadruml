#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr qint64 MaxLogSize = 5 * 1024 * 1024;

QMutex g_mutex;
QFile *g_file = nullptr;

QString levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return QStringLiteral("D");
    case QtInfoMsg: return QStringLiteral("I");
    case QtWarningMsg: return QStringLiteral("W");
    case QtCriticalMsg: return QStringLiteral("C");
    case QtFatalMsg: return QStringLiteral("F");
    }
    return QStringLiteral("?");
}

void handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString line = QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                   + QStringLiteral(" [") + levelName(type) + QStringLiteral("] ");
    if (context.category && std::strcmp(context.category, "default") != 0)
        line += QString::fromLatin1(context.category) + QStringLiteral(": ");
    line += message + QLatin1Char('\n');

    {
        QMutexLocker lock(&g_mutex);
        if (g_file) {
            g_file->write(line.toUtf8());
            g_file->flush();
        }
    }
#ifndef NDEBUG
    std::fputs(line.toLocal8Bit().constData(), stderr);
#endif
    if (type == QtFatalMsg)
        std::abort();
}

}

namespace Logger {

void install(const QString &logsDir)
{
    QDir().mkpath(logsDir);
    const QString path = logsDir + QStringLiteral("/quadrum.log");

    // Rotação simples: um arquivo anterior guardado como quadrum.log.1
    if (QFile::exists(path) && QFileInfo(path).size() > MaxLogSize) {
        QFile::remove(path + QStringLiteral(".1"));
        QFile::rename(path, path + QStringLiteral(".1"));
    }

    auto *file = new QFile(path);
    if (!file->open(QIODevice::Append)) {
        delete file;
        return;
    }
    g_file = file;
    qInstallMessageHandler(handler);
}

}
