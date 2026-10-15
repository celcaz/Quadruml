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

void ensureCreated();

}
