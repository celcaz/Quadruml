#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantMap>

namespace DiscInfo {

// Código do jogo a partir do SYSTEM.CNF de PS1/PS2.
// "BOOT2 = cdrom0:\SLUS_209.46;1" -> "SLUS-20946". Vazio se não achar.
QString serialFromSystemCnf(const QByteArray &systemCnf);

// Campos de um PARAM.SFO (PS3 e PS4): TITLE, TITLE_ID, CATEGORY, ...
// Textos viram QString; números, int. Vazio se o arquivo for inválido.
QVariantMap parseSfo(const QByteArray &sfo);

}
