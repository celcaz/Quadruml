#pragma once

#include <QList>
#include <QSet>
#include <QStringList>

#include <memory>

#include "ScannedGame.h"

class GameRepository;
class Ps2TitleDb;

// O que um provedor devolve de uma varredura.
struct ProviderScan
{
    QList<ScannedGame> changed;  // jogos novos ou com arquivos alterados
    QSet<QString> seenPaths;     // todos os arquivos encontrados, alterados ou não
    int unchanged = 0;           // arquivos pulados por não terem mudado
};

// PS2: imagens .iso; o código vem do SYSTEM.CNF e o nome, do GameIndex.yaml do PCSX2.
// A tabela de nomes (~2,7 MB) só é lida se aparecer algum jogo novo ou alterado.
class Ps2Provider
{
public:
    static constexpr const char *Id = "ps2";

    explicit Ps2Provider(QString gameIndexPath);
    ~Ps2Provider();
    ProviderScan scan(const QStringList &folders, const GameRepository &repository);

private:
    const Ps2TitleDb &titles();

    QString m_gameIndexPath;
    std::unique_ptr<Ps2TitleDb> m_titles;
};

// PS3: pastas de jogo (PS3_GAME/PARAM.SFO) e imagens .iso; código e nome vêm do PARAM.SFO.
class Ps3Provider
{
public:
    static constexpr const char *Id = "ps3";

    ProviderScan scan(const QStringList &folders, const GameRepository &repository) const;
};
