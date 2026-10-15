#include "LanguageService.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QQmlEngine>

Q_LOGGING_CATEGORY(lcLanguage, "quadrum.language")

namespace {
LanguageService *s_instance = nullptr;
}

LanguageService::LanguageService(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

LanguageService::~LanguageService()
{
    s_instance = nullptr;
}

LanguageService *LanguageService::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void LanguageService::setLanguage(const QString &language)
{
    if (language == m_language)
        return;

    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }

    // Também para pt_BR: o arquivo dele traz as formas de plural ("1 hora", "34 horas").
    if (m_translator.load(QStringLiteral(":/i18n/quadrum_") + language)) {
        QCoreApplication::installTranslator(&m_translator);
        m_installed = true;
    } else if (language != sourceLanguage()) {
        qCWarning(lcLanguage) << "tradução não encontrada para" << language << "- mantendo pt_BR";
        if (m_language == sourceLanguage())
            return;
    }

    m_language = (m_installed || language == sourceLanguage()) ? language : sourceLanguage();
    if (m_engine)
        m_engine->retranslate();
    qCInfo(lcLanguage) << "idioma:" << m_language;
    emit languageChanged();
}
