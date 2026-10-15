#include "Ps2TitleDb.h"

#include <QFile>
#include <QRegularExpression>

namespace {

// Valor entre aspas do YAML, com \" e \\ desfeitos.
QString unquote(QStringView value)
{
    value = value.trimmed();
    if (value.size() >= 2 && value.front() == u'"' && value.back() == u'"')
        value = value.sliced(1, value.size() - 2);
    QString out;
    out.reserve(value.size());
    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value[i] == u'\\' && i + 1 < value.size())
            ++i;
        out.append(value[i]);
    }
    return out;
}

}

bool Ps2TitleDb::load(const QString &gameIndexPath)
{
    QFile file(gameIndexPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    loadFromText(file.readAll());
    return isLoaded();
}

void Ps2TitleDb::loadFromText(const QByteArray &yaml)
{
    // Leitura linha a linha do formato do GameIndex.yaml:
    //   SLUS-20946:
    //     name: "..."
    //     name-en: "..."   (jogos japoneses: nome em inglês)
    //     region: "NTSC-U"
    static const QRegularExpression serialLine(QStringLiteral(R"(^([A-Z]{4}-\d{5}):\s*$)"));

    m_titles.clear();
    QString serial;
    QString name;
    QString nameEn;
    QString region;

    auto flush = [&] {
        if (!serial.isEmpty()) {
            const QString chosen = nameEn.isEmpty() ? name : nameEn;
            if (!chosen.isEmpty())
                m_titles.insert(serial, {chosen, region});
        }
        serial.clear();
        name.clear();
        nameEn.clear();
        region.clear();
    };

    const QList<QByteArray> lines = yaml.split('\n');
    for (const QByteArray &raw : lines) {
        QString line = QString::fromUtf8(raw);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.trimmed().isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (!line.startsWith(QLatin1Char(' '))) {
            const QRegularExpressionMatch match = serialLine.match(line.trimmed());
            flush();
            if (match.hasMatch())
                serial = match.captured(1);
            continue;
        }
        if (serial.isEmpty())
            continue;
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1String("name:")))
            name = unquote(QStringView(trimmed).sliced(5));
        else if (trimmed.startsWith(QLatin1String("name-en:")))
            nameEn = unquote(QStringView(trimmed).sliced(8));
        else if (trimmed.startsWith(QLatin1String("region:")))
            region = unquote(QStringView(trimmed).sliced(7));
    }
    flush();
}

const Ps2TitleDb::Title *Ps2TitleDb::find(const QString &serial) const
{
    const auto it = m_titles.constFind(serial);
    return it == m_titles.cend() ? nullptr : &it.value();
}
