#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

struct ScanConfig
{
    QStringList ps2Folders;
    QStringList ps3Folders;
    QString pcsx2Dir;   // para achar o GameIndex.yaml (nomes dos jogos de PS2)
    QString steamRoot;  // pasta da Steam; vazio = não ler a Steam
};

struct ScanSummary
{
    int changed = 0;    // jogos novos ou atualizados
    int unchanged = 0;  // arquivos pulados (sem mudança)
    int removed = 0;    // arquivos que sumiram das pastas
    int pending = 0;    // jogos com identificação pendente
    QMap<QString, int> byPlatform;
    qint64 elapsedMs = 0;
};

namespace ScanService {

// Varre todas as pastas configuradas e atualiza o banco. Abre a própria conexão
// com o banco, então pode rodar em outra thread sem travar a interface.
ScanSummary run(const QString &databasePath, const ScanConfig &config);

}
