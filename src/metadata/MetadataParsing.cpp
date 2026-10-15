#include "MetadataParsing.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTimeZone>

namespace MetadataParsing {

QString cleanTitle(const QString &title)
{
    QString clean = title;
    static const QRegularExpression brackets(QStringLiteral(R"(\s*[\(\[][^\)\]]*[\)\]])"));
    clean.remove(brackets);
    clean.remove(QRegularExpression(QStringLiteral("[™®©]")));
    return clean.simplified();
}

QString normalizedTitle(const QString &title)
{
    QString text = cleanTitle(title).normalized(QString::NormalizationForm_D).toLower();
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        if (c.isLetterOrNumber())
            out.append(c);
    }
    return out;
}

int pickSgdbGame(const QJsonArray &results, const QString &title)
{
    if (results.isEmpty())
        return 0;
    const QString wanted = normalizedTitle(title);
    for (const QJsonValue &value : results) {
        const QJsonObject game = value.toObject();
        if (normalizedTitle(game.value(QStringLiteral("name")).toString()) == wanted)
            return game.value(QStringLiteral("id")).toInt();
    }
    // Sem nome idêntico: a busca já vem ordenada por relevância.
    return results.first().toObject().value(QStringLiteral("id")).toInt();
}

QString pickSgdbImage(const QJsonArray &images)
{
    for (const QJsonValue &value : images) {
        const QJsonObject image = value.toObject();
        if (image.value(QStringLiteral("nsfw")).toBool() || image.value(QStringLiteral("humor")).toBool())
            continue;
        const QString url = image.value(QStringLiteral("url")).toString();
        if (!url.isEmpty())
            return url;
    }
    return {};
}

QString stripHtml(QString text)
{
    text.replace(QRegularExpression(QStringLiteral("<br\\s*/?>"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("\n"));
    text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    static const QList<QPair<QString, QString>> entities{
        {QStringLiteral("&quot;"), QStringLiteral("\"")}, {QStringLiteral("&#39;"), QStringLiteral("'")},
        {QStringLiteral("&apos;"), QStringLiteral("'")},  {QStringLiteral("&lt;"), QStringLiteral("<")},
        {QStringLiteral("&gt;"), QStringLiteral(">")},    {QStringLiteral("&nbsp;"), QStringLiteral(" ")},
        {QStringLiteral("&amp;"), QStringLiteral("&")},
    };
    for (const auto &[entity, replacement] : entities)
        text.replace(entity, replacement);
    return text.trimmed();
}

GameInfo parseSteamAppDetails(const QByteArray &json, const QString &appId)
{
    GameInfo info;
    const QJsonObject app = QJsonDocument::fromJson(json).object().value(appId).toObject();
    if (!app.value(QStringLiteral("success")).toBool())
        return info;
    const QJsonObject data = app.value(QStringLiteral("data")).toObject();

    info.description = stripHtml(data.value(QStringLiteral("short_description")).toString());
    QStringList genres;
    for (const QJsonValue &genre : data.value(QStringLiteral("genres")).toArray())
        genres.append(genre.toObject().value(QStringLiteral("description")).toString());
    info.genre = genres.mid(0, 2).join(QStringLiteral(", "));
    info.releaseDate = data.value(QStringLiteral("release_date")).toObject().value(QStringLiteral("date")).toString();
    info.found = !info.description.isEmpty();
    return info;
}

GameInfo parseIgdbGames(const QByteArray &json, const QString &title)
{
    GameInfo info;
    const QJsonArray games = QJsonDocument::fromJson(json).array();
    if (games.isEmpty())
        return info;

    QJsonObject chosen = games.first().toObject();
    const QString wanted = normalizedTitle(title);
    for (const QJsonValue &value : games) {
        if (normalizedTitle(value.toObject().value(QStringLiteral("name")).toString()) == wanted) {
            chosen = value.toObject();
            break;
        }
    }

    info.description = chosen.value(QStringLiteral("summary")).toString().trimmed();
    QStringList genres;
    for (const QJsonValue &genre : chosen.value(QStringLiteral("genres")).toArray())
        genres.append(genre.toObject().value(QStringLiteral("name")).toString());
    info.genre = genres.mid(0, 2).join(QStringLiteral(", "));
    const qint64 timestamp = chosen.value(QStringLiteral("first_release_date")).toInteger();
    if (timestamp > 0)
        info.releaseDate = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::UTC).date().toString(Qt::ISODate);
    info.found = !info.description.isEmpty();
    return info;
}

int igdbPlatform(const QString &platform)
{
    if (platform == QLatin1String("ps1")) return 7;
    if (platform == QLatin1String("ps2")) return 8;
    if (platform == QLatin1String("ps3")) return 9;
    if (platform == QLatin1String("ps4")) return 48;
    if (platform == QLatin1String("pc")) return 6;
    return 0;
}

}
