#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

#include "disc/DiscInfo.h"
#include "disc/Iso9660.h"
#include "disc/SectorReader.h"
#include "titles/Ps2TitleDb.h"

namespace {

constexpr int S = SectorReader::SectorSize;

// Registro de diretório ISO 9660 mínimo.
QByteArray dirRecord(const QByteArray &name, quint32 lba, quint32 size, bool directory)
{
    const int length = 33 + int(name.size()) + (name.size() % 2 == 0 ? 1 : 0);
    QByteArray record(length, '\0');
    record[0] = char(length);
    qToLittleEndian<quint32>(lba, record.data() + 2);
    qToLittleEndian<quint32>(size, record.data() + 10);
    record[25] = directory ? 0x02 : 0x00;
    record[32] = char(name.size());
    record.replace(33, name.size(), name);
    return record;
}

void writeSector(QByteArray &image, int lba, const QByteArray &data)
{
    image.replace(lba * S, data.size(), data);
}

// Imagem com: raiz (setor 18) -> SYSTEM.CNF (19) e PS3_GAME/ (20) -> PARAM.SFO (21).
QByteArray buildIso(const QByteArray &systemCnf, const QByteArray &paramSfo)
{
    QByteArray image(22 * S, '\0');

    QByteArray pvd(S, '\0');
    pvd[0] = 1;
    pvd.replace(1, 5, "CD001");
    pvd.replace(156, 34, dirRecord(QByteArray(1, '\0'), 18, S, true).left(34));
    writeSector(image, 16, pvd);

    QByteArray root;
    root += dirRecord(QByteArray(1, '\0'), 18, S, true);
    root += dirRecord(QByteArray(1, '\1'), 18, S, true);
    root += dirRecord("SYSTEM.CNF;1", 19, quint32(systemCnf.size()), false);
    root += dirRecord("PS3_GAME", 20, S, true);
    writeSector(image, 18, root);
    writeSector(image, 19, systemCnf);

    QByteArray ps3Dir;
    ps3Dir += dirRecord(QByteArray(1, '\0'), 20, S, true);
    ps3Dir += dirRecord(QByteArray(1, '\1'), 18, S, true);
    ps3Dir += dirRecord("PARAM.SFO;1", 21, quint32(paramSfo.size()), false);
    writeSector(image, 20, ps3Dir);
    writeSector(image, 21, paramSfo);
    return image;
}

// PARAM.SFO mínimo com campos de texto.
QByteArray buildSfo(const QList<QPair<QByteArray, QByteArray>> &fields)
{
    QByteArray keys;
    QByteArray values;
    QByteArray index;
    for (const auto &[key, value] : fields) {
        QByteArray entry(16, '\0');
        qToLittleEndian<quint16>(quint16(keys.size()), entry.data());
        qToLittleEndian<quint16>(0x0204, entry.data() + 2);
        qToLittleEndian<quint32>(quint32(value.size() + 1), entry.data() + 4);
        qToLittleEndian<quint32>(quint32(value.size() + 1), entry.data() + 8);
        qToLittleEndian<quint32>(quint32(values.size()), entry.data() + 12);
        index += entry;
        keys += key + '\0';
        values += value + '\0';
    }
    QByteArray header(20, '\0');
    header.replace(0, 4, QByteArray("\0PSF", 4));
    qToLittleEndian<quint32>(0x101, header.data() + 4);
    qToLittleEndian<quint32>(quint32(20 + index.size()), header.data() + 8);
    qToLittleEndian<quint32>(quint32(20 + index.size() + keys.size()), header.data() + 12);
    qToLittleEndian<quint32>(quint32(fields.size()), header.data() + 16);
    return header + index + keys + values;
}

}

class TestDisc : public QObject
{
    Q_OBJECT

private slots:
    void serialFromPs2SystemCnf()
    {
        QCOMPARE(DiscInfo::serialFromSystemCnf("BOOT2 = cdrom0:\\SLUS_209.46;1\r\nVER = 1.00\r\n"),
                 QStringLiteral("SLUS-20946"));
    }

    void serialFromPs1SystemCnf()
    {
        QCOMPARE(DiscInfo::serialFromSystemCnf("BOOT = cdrom:\\SCUS_941.63;1\r\nTCB = 4\r\n"),
                 QStringLiteral("SCUS-94163"));
        QCOMPARE(DiscInfo::serialFromSystemCnf("BOOT=cdrom:SLES_123.45;1"), QStringLiteral("SLES-12345"));
    }

    void serialMissing()
    {
        QVERIFY(DiscInfo::serialFromSystemCnf("nada aqui").isEmpty());
    }

    void parsesSfo()
    {
        const QVariantMap sfo = DiscInfo::parseSfo(buildSfo({{"TITLE", "Jogo de Teste"}, {"TITLE_ID", "BLUS30443"}}));
        QCOMPARE(sfo.value(QStringLiteral("TITLE")).toString(), QStringLiteral("Jogo de Teste"));
        QCOMPARE(sfo.value(QStringLiteral("TITLE_ID")).toString(), QStringLiteral("BLUS30443"));
        QVERIFY(DiscInfo::parseSfo("lixo").isEmpty());
    }

    void readsFilesFromIso()
    {
        const QByteArray systemCnf = "BOOT2 = cdrom0:\\SLUS_209.46;1\r\n";
        const QByteArray sfo = buildSfo({{"TITLE", "Teste"}, {"TITLE_ID", "BLUS00001"}});

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("teste.iso"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(buildIso(systemCnf, sfo));
        file.close();

        IsoFileReader reader(path);
        QVERIFY(reader.open());
        Iso9660 iso(reader);
        QVERIFY(iso.open());

        const auto cnf = iso.readFile(QStringLiteral("system.cnf")); // sem diferenciar maiúsculas
        QVERIFY(cnf.has_value());
        QCOMPARE(*cnf, systemCnf);

        const auto param = iso.readFile(QStringLiteral("PS3_GAME/PARAM.SFO"));
        QVERIFY(param.has_value());
        QCOMPARE(DiscInfo::parseSfo(*param).value(QStringLiteral("TITLE_ID")).toString(), QStringLiteral("BLUS00001"));

        QVERIFY(!iso.readFile(QStringLiteral("NAO_EXISTE.BIN")).has_value());
    }

    void rejectsNonIso()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("falso.iso"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray(20 * S, 'x'));
        file.close();

        IsoFileReader reader(path);
        QVERIFY(reader.open());
        Iso9660 iso(reader);
        QVERIFY(!iso.open());
    }

    void parsesGameIndex()
    {
        Ps2TitleDb db;
        db.loadFromText(
            "# comentário\n"
            "SLUS-20946:\n"
            "  name: \"Shadow of Rome\"\n"
            "  region: \"NTSC-U\"\n"
            "  gameFixes:\n"
            "    - SoftwareRendererFMVHack\n"
            "SLPM-12345:\n"
            "  name: \"\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88\"\n"
            "  name-en: \"Test \\\"Game\\\"\"\n"
            "  region: \"NTSC-J\"\n");
        QCOMPARE(db.size(), 2);
        QCOMPARE(db.find(QStringLiteral("SLUS-20946"))->name, QStringLiteral("Shadow of Rome"));
        QCOMPARE(db.find(QStringLiteral("SLUS-20946"))->region, QStringLiteral("NTSC-U"));
        QCOMPARE(db.find(QStringLiteral("SLPM-12345"))->name, QStringLiteral("Test \"Game\""));
        QVERIFY(db.find(QStringLiteral("SLUS-00000")) == nullptr);
    }
};

QTEST_GUILESS_MAIN(TestDisc)
#include "tst_disc.moc"
