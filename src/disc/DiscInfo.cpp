#include "DiscInfo.h"

#include <QRegularExpression>
#include <QtEndian>

namespace DiscInfo {

QString serialFromSystemCnf(const QByteArray &systemCnf)
{
    // PS2 usa BOOT2 e PS1 usa BOOT; o caminho pode ou não ter "\" e ";1".
    static const QRegularExpression pattern(
        QStringLiteral(R"(BOOT2?\s*=\s*cdrom0?:\\?\\?([A-Z]{4})[_-](\d{3})\.?(\d{2}))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(QString::fromLatin1(systemCnf));
    if (!match.hasMatch())
        return {};
    return match.captured(1).toUpper() + QLatin1Char('-') + match.captured(2) + match.captured(3);
}

QVariantMap parseSfo(const QByteArray &sfo)
{
    // Formato: cabeçalho de 20 bytes, uma tabela de índices (16 bytes cada),
    // uma tabela de nomes e uma tabela de valores.
    constexpr int HeaderSize = 20;
    constexpr int IndexSize = 16;
    if (sfo.size() < HeaderSize || !sfo.startsWith(QByteArray("\0PSF", 4)))
        return {};

    const char *data = sfo.constData();
    const quint32 keyTable = qFromLittleEndian<quint32>(data + 8);
    const quint32 dataTable = qFromLittleEndian<quint32>(data + 12);
    const quint32 count = qFromLittleEndian<quint32>(data + 16);
    if (HeaderSize + qint64(count) * IndexSize > sfo.size())
        return {};

    QVariantMap fields;
    for (quint32 i = 0; i < count; ++i) {
        const char *index = data + HeaderSize + i * IndexSize;
        const quint16 keyOffset = qFromLittleEndian<quint16>(index);
        const quint16 format = qFromLittleEndian<quint16>(index + 2);
        const quint32 length = qFromLittleEndian<quint32>(index + 4);
        const quint32 dataOffset = qFromLittleEndian<quint32>(index + 12);

        const qint64 keyStart = qint64(keyTable) + keyOffset;
        const qint64 valueStart = qint64(dataTable) + dataOffset;
        if (keyStart >= sfo.size() || valueStart + length > sfo.size())
            continue;

        const QString key = QString::fromLatin1(data + keyStart);
        if (format == 0x0404 && length >= 4) {
            fields.insert(key, qFromLittleEndian<qint32>(data + valueStart));
        } else {
            // 0x0204 (texto terminado em zero) e 0x0004 (texto sem terminador)
            QByteArray text(data + valueStart, length);
            const qsizetype end = text.indexOf('\0');
            if (end >= 0)
                text.truncate(end);
            fields.insert(key, QString::fromUtf8(text));
        }
    }
    return fields;
}

}
