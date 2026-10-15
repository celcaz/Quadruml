#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// Carrega um tema de themes/<nome>/: theme.json (cores, tamanhos, tempos,
// nomes de arquivos) e os recursos (fontes, imagens, sons).
// As telas nunca acessam o disco: pedem tudo por aqui, via Theme.qml.
class ThemeService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(bool found READ found NOTIFY changed)
    Q_PROPERTY(QVariantMap tokens READ tokens NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    // Sem construtor padrão de propósito: assim o QML usa sempre create()
    // e recebe a instância criada no main, em vez de construir outra.
    explicit ThemeService(QObject *parent);
    ~ThemeService() override;

    static ThemeService *create(QQmlEngine *, QJSEngine *);

    bool load(const QString &name);

    QString name() const { return m_name; }
    bool found() const { return !m_dir.isEmpty(); }
    QVariantMap tokens() const { return m_tokens; }
    QString fontFamily() const { return m_fontFamily; }
    int revision() const { return m_revision; }

    // Caminho relativo à pasta do tema -> URL do arquivo, ou vazio se não existir.
    Q_INVOKABLE QUrl asset(const QString &relativePath) const;
    // Arquivo declarado em theme.json, ex.: image("logo") lê "images": { "logo": ... }.
    Q_INVOKABLE QUrl image(const QString &key) const;
    Q_INVOKABLE QUrl sound(const QString &key) const;

signals:
    void changed();

private:
    QUrl assetFromGroup(const QString &group, const QString &key) const;
    void loadFonts();

    QString m_name;
    QString m_dir;
    QVariantMap m_tokens;
    QString m_fontFamily;
    int m_revision = 0;
};
