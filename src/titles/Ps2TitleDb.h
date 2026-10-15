#pragma once

#include <QHash>
#include <QString>

// Tabela código -> nome dos jogos de PS2, lida do GameIndex.yaml do PCSX2
// instalado pelo usuário (pasta resources/). Nada é copiado para o hub.
class Ps2TitleDb
{
public:
    struct Title
    {
        QString name;
        QString region;
    };

    bool load(const QString &gameIndexPath);
    bool isLoaded() const { return !m_titles.isEmpty(); }
    int size() const { return int(m_titles.size()); }

    // nullptr se o código não estiver na tabela.
    const Title *find(const QString &serial) const;

    // Lê o texto diretamente (usado pelos testes).
    void loadFromText(const QByteArray &yaml);

private:
    QHash<QString, Title> m_titles;
};
