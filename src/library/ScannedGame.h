#pragma once

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>

struct ScannedFile
{
    QString path;
    QString format;      // "iso", "chd", "pasta", ...
    qint64 size = 0;
    QDateTime modified;
    int disc = 1;
};

// Jogo encontrado por um provedor numa varredura, antes de ir para o banco.
struct ScannedGame
{
    QString platform;    // "ps2", "ps3", ...
    QString provider;    // de onde veio; junto com externalId identifica o jogo
    QString externalId;  // código do disco (SLUS-20946) ou ID da loja
    QString title;
    bool identificationPending = false;
    QList<ScannedFile> files;
    // Artes encontradas no próprio PC (ex.: cache da Steam): tipo -> caminho.
    // Tipos: "capa", "fundo", "logo".
    QMap<QString, QString> media;
};

namespace ScanUtil {

// "Shadow of Rome (USA).iso" -> "Shadow of Rome (USA)"
inline QString titleFromFileName(const QString &fileName)
{
    const qsizetype dot = fileName.lastIndexOf(QLatin1Char('.'));
    return dot > 0 ? fileName.left(dot) : fileName;
}

}
