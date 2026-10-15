#pragma once

#include <QFile>
#include <QString>

// Lê setores de 2048 bytes de uma imagem de disco. Cada formato (ISO, BIN de
// PS1, CHD) tem a sua implementação; o leitor de ISO 9660 não sabe a diferença.
class SectorReader
{
public:
    static constexpr int SectorSize = 2048;

    virtual ~SectorReader() = default;
    virtual bool readSector(quint32 lba, char *out) = 0;
};

// Imagem .iso comum: setores de 2048 bytes, um atrás do outro.
class IsoFileReader : public SectorReader
{
public:
    explicit IsoFileReader(const QString &path)
        : m_file(path)
    {
    }

    bool open() { return m_file.open(QIODevice::ReadOnly); }

    bool readSector(quint32 lba, char *out) override
    {
        return m_file.seek(qint64(lba) * SectorSize) && m_file.read(out, SectorSize) == SectorSize;
    }

private:
    QFile m_file;
};
