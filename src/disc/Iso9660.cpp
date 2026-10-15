#include "Iso9660.h"

#include "SectorReader.h"

#include <QtEndian>

namespace {

constexpr quint32 PrimaryVolumeDescriptorLba = 16;
constexpr int RootRecordOffset = 156;
constexpr quint32 MaxDirectorySize = 4 * 1024 * 1024;

quint32 le32(const char *p)
{
    return qFromLittleEndian<quint32>(p);
}

// "SYSTEM.CNF;1" -> "SYSTEM.CNF"; "DIR." -> "DIR"
QString cleanName(const char *data, int length)
{
    QString name = QString::fromLatin1(data, length);
    const qsizetype version = name.indexOf(QLatin1Char(';'));
    if (version >= 0)
        name.truncate(version);
    if (name.endsWith(QLatin1Char('.')))
        name.chop(1);
    return name;
}

}

Iso9660::Iso9660(SectorReader &reader)
    : m_reader(reader)
{
}

bool Iso9660::open()
{
    char sector[SectorReader::SectorSize];
    if (!m_reader.readSector(PrimaryVolumeDescriptorLba, sector))
        return false;
    // Tipo 1 (descritor primário) + identificador "CD001"
    if (sector[0] != 1 || QByteArray(sector + 1, 5) != "CD001")
        return false;

    const char *root = sector + RootRecordOffset;
    m_root.lba = le32(root + 2);
    m_root.size = le32(root + 10);
    m_root.directory = true;
    m_open = true;
    return true;
}

std::optional<QByteArray> Iso9660::readFile(const QString &path, qint64 maxSize)
{
    const auto entry = find(path);
    if (!entry || entry->directory || entry->size > maxSize)
        return std::nullopt;

    QByteArray data;
    data.reserve(entry->size);
    char sector[SectorReader::SectorSize];
    const quint32 sectors = (entry->size + SectorReader::SectorSize - 1) / SectorReader::SectorSize;
    for (quint32 i = 0; i < sectors; ++i) {
        if (!m_reader.readSector(entry->lba + i, sector))
            return std::nullopt;
        data.append(sector, SectorReader::SectorSize);
    }
    data.truncate(entry->size);
    return data;
}

std::optional<Iso9660::Entry> Iso9660::find(const QString &path)
{
    if (!m_open)
        return std::nullopt;
    Entry current = m_root;
    for (const QString &part : path.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        if (!current.directory)
            return std::nullopt;
        const auto next = findInDirectory(current, part);
        if (!next)
            return std::nullopt;
        current = *next;
    }
    return current;
}

std::optional<Iso9660::Entry> Iso9660::findInDirectory(const Entry &directory, const QString &name)
{
    if (directory.size > MaxDirectorySize)
        return std::nullopt;

    char sector[SectorReader::SectorSize];
    const quint32 sectors = (directory.size + SectorReader::SectorSize - 1) / SectorReader::SectorSize;
    for (quint32 s = 0; s < sectors; ++s) {
        if (!m_reader.readSector(directory.lba + s, sector))
            return std::nullopt;

        int offset = 0;
        while (offset < SectorReader::SectorSize) {
            const int recordLength = static_cast<unsigned char>(sector[offset]);
            if (recordLength == 0)
                break; // resto do setor vazio; os registros continuam no próximo
            if (offset + recordLength > SectorReader::SectorSize || recordLength < 34)
                break;

            const char *record = sector + offset;
            const int nameLength = static_cast<unsigned char>(record[32]);
            if (33 + nameLength <= recordLength) {
                const QString entryName = cleanName(record + 33, nameLength);
                if (entryName.compare(name, Qt::CaseInsensitive) == 0) {
                    Entry entry;
                    entry.lba = le32(record + 2);
                    entry.size = le32(record + 10);
                    entry.directory = (record[25] & 0x02) != 0;
                    return entry;
                }
            }
            offset += recordLength;
        }
    }
    return std::nullopt;
}
