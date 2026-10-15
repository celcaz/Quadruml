#pragma once

#include <QByteArray>
#include <QVariantMap>

namespace Vdf {

// Lê o formato de texto KeyValues da Steam (libraryfolders.vdf, appmanifest_*.acf):
//   "chave" "valor"   e   "chave" { ... }
// Blocos viram QVariantMap aninhados; valores, QString.
QVariantMap parse(const QByteArray &text);

}
