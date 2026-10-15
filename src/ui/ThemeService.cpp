#include "ThemeService.h"

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include "core/Paths.h"

Q_LOGGING_CATEGORY(lcTheme, "quadrum.theme")

namespace {
ThemeService *s_instance = nullptr;
}

ThemeService::ThemeService(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

ThemeService::~ThemeService()
{
    s_instance = nullptr;
}

ThemeService *ThemeService::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool ThemeService::load(const QString &name)
{
    m_name = name;
    m_dir.clear();
    m_tokens.clear();
    m_fontFamily.clear();

    for (const QString &base : Paths::themeSearchDirs()) {
        const QDir candidate(base + QLatin1Char('/') + name);
        if (candidate.exists()) {
            m_dir = candidate.absolutePath();
            break;
        }
    }

    if (m_dir.isEmpty()) {
        qCWarning(lcTheme) << "tema" << name << "não encontrado em" << Paths::themeSearchDirs()
                           << "- usando valores padrão";
    } else {
        QFile file(m_dir + QStringLiteral("/theme.json"));
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError error;
            const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
            if (doc.isObject())
                m_tokens = doc.object().toVariantMap();
            else
                qCWarning(lcTheme) << "theme.json inválido:" << error.errorString();
        } else {
            qCWarning(lcTheme) << "tema" << name << "sem theme.json; usando valores padrão";
        }
        loadFonts();
        qCInfo(lcTheme) << "tema carregado:" << name << "de" << m_dir;
    }

    ++m_revision;
    emit changed();
    return found();
}

void ThemeService::loadFonts()
{
    const QVariantMap fonts = m_tokens.value(QStringLiteral("fonts")).toMap();
    for (auto it = fonts.cbegin(); it != fonts.cend(); ++it) {
        const QString path = QDir(m_dir).filePath(it.value().toString());
        if (!QFile::exists(path)) {
            qCInfo(lcTheme) << "fonte ausente no tema (opcional):" << it.key();
            continue;
        }
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0) {
            qCWarning(lcTheme) << "fonte não carregada:" << path;
            continue;
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (it.key() == QLatin1String("regular") && !families.isEmpty())
            m_fontFamily = families.constFirst();
    }

    // A fonte do tema vira a padrão de todo texto do app.
    if (!m_fontFamily.isEmpty())
        QGuiApplication::setFont(QFont(m_fontFamily));
}

QUrl ThemeService::asset(const QString &relativePath) const
{
    if (m_dir.isEmpty() || relativePath.isEmpty())
        return {};
    const QString path = QDir(m_dir).filePath(relativePath);
    return QFile::exists(path) ? QUrl::fromLocalFile(path) : QUrl();
}

QUrl ThemeService::assetFromGroup(const QString &group, const QString &key) const
{
    return asset(m_tokens.value(group).toMap().value(key).toString());
}

QUrl ThemeService::image(const QString &key) const
{
    return assetFromGroup(QStringLiteral("images"), key);
}

QUrl ThemeService::sound(const QString &key) const
{
    return assetFromGroup(QStringLiteral("sounds"), key);
}
