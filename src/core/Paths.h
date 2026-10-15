#pragma once

#include <QString>
#include <QStringList>

// Pastas e arquivos do hub. Tudo que é do usuário fica em dataRoot().
namespace Paths {

QString dataRoot();
QString configFile();
QString databaseFile();
QString logsDir();
QString mediaDir();

// Onde procurar temas, em ordem de prioridade.
QStringList themeSearchDirs();

// Arquivo que acompanha o app (ao lado do .exe ou, no desenvolvimento, em assets/).
// Devolve vazio se não existir em nenhum dos dois.
QString findBundledFile(const QString &fileName, const QString &assetsSubdir);

void ensureCreated();

}
