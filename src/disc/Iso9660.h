#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

class SectorReader;

// Sistema de arquivos ISO 9660 (CD e DVD de PS1, PS2 e PS3). Só leitura, e só o
// necessário para achar arquivos pequenos como SYSTEM.CNF e PARAM.SFO.
class Iso9660
{
public:
    explicit Iso9660(SectorReader &reader);

    bool open();

    // Caminho com "/" a partir da raiz, sem diferenciar maiúsculas: "PS3_GAME/PARAM.SFO".
    std::optional<QByteArray> readFile(const QString &path, qint64 maxSize = 1024 * 1024);

private:
    struct Entry
    {
        quint32 lba = 0;
        quint32 size = 0;
        bool directory = false;
    };

    std::optional<Entry> find(const QString &path);
    std::optional<Entry> findInDirectory(const Entry &directory, const QString &name);

    SectorReader &m_reader;
    Entry m_root;
    bool m_open = false;
};
