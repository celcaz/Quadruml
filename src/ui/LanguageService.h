#pragma once

#include <QObject>
#include <QTranslator>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Troca o idioma da interface na hora, sem reiniciar.
// Os textos do código são escritos em português (pt_BR); o inglês vem de i18n/quadrum_en.ts.
class LanguageService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    explicit LanguageService(QObject *parent);
    ~LanguageService() override;

    static LanguageService *create(QQmlEngine *, QJSEngine *);
    static QString sourceLanguage() { return QStringLiteral("pt_BR"); }

    void setEngine(QQmlEngine *engine) { m_engine = engine; }

    QString language() const { return m_language; }
    void setLanguage(const QString &language);

signals:
    void languageChanged();

private:
    QQmlEngine *m_engine = nullptr;
    QTranslator m_translator;
    bool m_installed = false;
    QString m_language;
};
