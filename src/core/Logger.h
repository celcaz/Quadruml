#pragma once

#include <QString>

// Grava todas as mensagens do Qt (qInfo, qWarning...) em logs/quadrum.log,
// com data e hora em milissegundos, para medir tempos e investigar erros.
namespace Logger {

void install(const QString &logsDir);

}
