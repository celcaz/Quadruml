#pragma once

#include <QString>

// Consultas a processos do sistema. A implementação de cada sistema fica em
// Processes.cpp; no Linux (versão futura) entra outra implementação aqui.
namespace Processes {

// Ex.: isRunning("steam.exe"). Sem diferenciar maiúsculas de minúsculas.
bool isRunning(const QString &executableName);

}
