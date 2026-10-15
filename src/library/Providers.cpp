#include "Providers.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QLoggingCategory>

#include <optional>

#include "GameRepository.h"
#include "Vdf.h"
#include "disc/DiscInfo.h"
#include "disc/Iso9660.h"
#include "disc/SectorReader.h"
#include "titles/Ps2TitleDb.h"

Q_LOGGING_CATEGORY(lcScan, "quadrum.scan")

namespace {

ScannedFile fileFrom(const QFileInfo &info, const QString &format)
{
    ScannedFile file;
    file.path = QDir::fromNativeSeparators(info.absoluteFilePath());
    file.format = format;
    file.size = info.size();
    file.modified = info.lastModified();
    return file;
}

std::optional<QByteArray> readFromIso(const QString &path, const QString &innerPath)
{
    IsoFileReader reader(path);
    if (!reader.open())
        return std::nullopt;
    Iso9660 iso(reader);
    if (!iso.open())
        return std::nullopt;
    return iso.readFile(innerPath);
}

// Junta jogos com o mesmo código (vários arquivos do mesmo jogo viram um só).
void addOrMerge(QList<ScannedGame> &games, QHash<QString, qsizetype> &byId, ScannedGame game)
{
    const auto it = byId.constFind(game.externalId);
    if (it != byId.cend()) {
        games[it.value()].files.append(game.files);
        return;
    }
    byId.insert(game.externalId, games.size());
    games.append(std::move(game));
}

QString cleanSfoTitle(QString title)
{
    // Alguns títulos de PS3 têm quebra de linha (ex.: nome + subtítulo).
    return title.replace(QLatin1Char('\n'), QLatin1Char(' ')).simplified();
}

}

// --- PS2 --------------------------------------------------------------------

Ps2Provider::Ps2Provider(QString gameIndexPath)
    : m_gameIndexPath(std::move(gameIndexPath))
{
}

Ps2Provider::~Ps2Provider() = default;

const Ps2TitleDb &Ps2Provider::titles()
{
    if (!m_titles) {
        m_titles = std::make_unique<Ps2TitleDb>();
        if (m_titles->load(m_gameIndexPath))
            qCInfo(lcScan) << "tabela de nomes do PS2:" << m_titles->size() << "jogos";
        else
            qCWarning(lcScan) << "GameIndex.yaml do PCSX2 não encontrado em" << m_gameIndexPath
                              << "- jogos de PS2 ficarão com o nome do arquivo";
    }
    return *m_titles;
}

ProviderScan Ps2Provider::scan(const QStringList &folders, const GameRepository &repository)
{
    ProviderScan result;
    QHash<QString, qsizetype> byId;

    for (const QString &folder : folders) {
        QDirIterator it(folder, {QStringLiteral("*.iso")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo info(it.next());
            const ScannedFile file = fileFrom(info, QStringLiteral("iso"));
            result.seenPaths.insert(file.path);
            if (repository.isUnchanged(file.path, file.size, file.modified)) {
                ++result.unchanged;
                continue;
            }

            ScannedGame game;
            game.platform = QStringLiteral("ps2");
            game.provider = QString::fromLatin1(Id);
            game.files.append(file);

            const auto systemCnf = readFromIso(info.absoluteFilePath(), QStringLiteral("SYSTEM.CNF"));
            const QString serial = systemCnf ? DiscInfo::serialFromSystemCnf(*systemCnf) : QString();
            const Ps2TitleDb::Title *title = serial.isEmpty() ? nullptr : titles().find(serial);

            if (title) {
                game.externalId = serial;
                game.title = title->name;
            } else {
                // Sem código ou fora da tabela: usa o nome do arquivo e marca para revisão.
                game.externalId = serial.isEmpty() ? QStringLiteral("arquivo:") + info.fileName() : serial;
                game.title = ScanUtil::titleFromFileName(info.fileName());
                game.identificationPending = true;
            }
            qCInfo(lcScan) << "PS2:" << game.externalId << game.title << (game.identificationPending ? "(pendente)" : "");
            addOrMerge(result.changed, byId, std::move(game));
        }
    }
    return result;
}

// --- PS3 --------------------------------------------------------------------

ProviderScan Ps3Provider::scan(const QStringList &folders, const GameRepository &repository) const
{
    ProviderScan result;
    QHash<QString, qsizetype> byId;

    auto addGame = [&](const ScannedFile &file, const QVariantMap &sfo, const QString &fallbackName) {
        ScannedGame game;
        game.platform = QStringLiteral("ps3");
        game.provider = QString::fromLatin1(Id);
        game.files.append(file);
        const QString titleId = sfo.value(QStringLiteral("TITLE_ID")).toString();
        const QString title = cleanSfoTitle(sfo.value(QStringLiteral("TITLE")).toString());
        if (!titleId.isEmpty() && !title.isEmpty()) {
            game.externalId = titleId;
            game.title = title;
        } else {
            game.externalId = QStringLiteral("arquivo:") + fallbackName;
            game.title = fallbackName;
            game.identificationPending = true;
        }
        qCInfo(lcScan) << "PS3:" << game.externalId << game.title << (game.identificationPending ? "(pendente)" : "");
        addOrMerge(result.changed, byId, std::move(game));
    };

    // Pasta de jogo: <jogo>/PS3_GAME/PARAM.SFO (o PARAM.SFO é o arquivo de referência).
    auto tryGameFolder = [&](const QFileInfo &dir) -> bool {
        const QFileInfo sfoInfo(dir.absoluteFilePath() + QStringLiteral("/PS3_GAME/PARAM.SFO"));
        if (!sfoInfo.exists())
            return false;
        const ScannedFile file = fileFrom(sfoInfo, QStringLiteral("pasta"));
        result.seenPaths.insert(file.path);
        if (repository.isUnchanged(file.path, file.size, file.modified)) {
            ++result.unchanged;
            return true;
        }
        QFile sfoFile(sfoInfo.absoluteFilePath());
        const QByteArray sfo = sfoFile.open(QIODevice::ReadOnly) ? sfoFile.readAll() : QByteArray();
        addGame(file, DiscInfo::parseSfo(sfo), dir.fileName());
        return true;
    };

    auto tryIso = [&](const QFileInfo &info) {
        if (info.suffix().compare(QLatin1String("iso"), Qt::CaseInsensitive) != 0)
            return;
        const ScannedFile file = fileFrom(info, QStringLiteral("iso"));
        result.seenPaths.insert(file.path);
        if (repository.isUnchanged(file.path, file.size, file.modified)) {
            ++result.unchanged;
            return;
        }
        const auto sfo = readFromIso(info.absoluteFilePath(), QStringLiteral("PS3_GAME/PARAM.SFO"));
        addGame(file, sfo ? DiscInfo::parseSfo(*sfo) : QVariantMap(), ScanUtil::titleFromFileName(info.fileName()));
    };

    // Só dois níveis: <pasta>/<jogo> e <pasta>/<subpasta>/<jogo>. Não entra nas
    // pastas dos jogos, que no PS3 têm milhares de arquivos.
    const QDir::Filters filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
    for (const QString &folder : folders) {
        for (const QFileInfo &entry : QDir(folder).entryInfoList(filters)) {
            if (entry.isFile()) {
                tryIso(entry);
                continue;
            }
            if (tryGameFolder(entry))
                continue;
            for (const QFileInfo &inner : QDir(entry.absoluteFilePath()).entryInfoList(filters)) {
                if (inner.isFile())
                    tryIso(inner);
                else
                    tryGameFolder(inner);
            }
        }
    }
    return result;
}

// --- Steam ------------------------------------------------------------------

QStringList SteamProvider::libraryFolders(const QString &steamRoot)
{
    QStringList folders;
    QFile file(steamRoot + QStringLiteral("/steamapps/libraryfolders.vdf"));
    if (file.open(QIODevice::ReadOnly)) {
        const QVariantMap libraries = Vdf::parse(file.readAll()).value(QStringLiteral("libraryfolders")).toMap();
        for (const QVariant &library : libraries) {
            const QString path = library.toMap().value(QStringLiteral("path")).toString();
            if (!path.isEmpty())
                folders.append(QDir::cleanPath(QDir::fromNativeSeparators(path)) + QStringLiteral("/steamapps"));
        }
    }
    const QString own = QDir::cleanPath(steamRoot) + QStringLiteral("/steamapps");
    if (!folders.contains(own, Qt::CaseInsensitive))
        folders.prepend(own);
    return folders;
}

QMap<QString, QString> SteamProvider::localArtwork(const QString &steamRoot, const QString &appId)
{
    // Dois formatos de cache: o antigo, com arquivos direto em librarycache/<appid>/,
    // e o novo, com cada arte numa subpasta de nome aleatório.
    static const QList<QPair<QString, QStringList>> wanted{
        {QStringLiteral("capa"), {QStringLiteral("library_600x900.jpg"), QStringLiteral("library_capsule.jpg")}},
        {QStringLiteral("fundo"), {QStringLiteral("library_hero.jpg")}},
        {QStringLiteral("logo"), {QStringLiteral("logo.png")}},
    };

    QMap<QString, QString> found;
    const QString base = steamRoot + QStringLiteral("/appcache/librarycache/") + appId;
    if (!QDir(base).exists())
        return found;

    QStringList candidates{base};
    for (const QString &sub : QDir(base).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        candidates.append(base + QLatin1Char('/') + sub);

    for (const auto &[type, names] : wanted) {
        for (const QString &dir : candidates) {
            for (const QString &name : names) {
                const QString path = dir + QLatin1Char('/') + name;
                if (!found.contains(type) && QFileInfo::exists(path))
                    found.insert(type, path);
            }
        }
    }
    return found;
}

bool SteamProvider::isTool(const QString &appId, const QString &name)
{
    static const QSet<QString> toolIds{
        QStringLiteral("228980"),  // Steamworks Common Redistributables
        QStringLiteral("250820"),  // SteamVR
        QStringLiteral("1070560"), // Steam Linux Runtime
        QStringLiteral("1391110"), // Steam Linux Runtime - Soldier
        QStringLiteral("1628350"), // Steam Linux Runtime - Sniper
        QStringLiteral("1826330"), // Proton EasyAntiCheat Runtime
        QStringLiteral("1161040"), // Proton BattlEye Runtime
    };
    return toolIds.contains(appId) || name.startsWith(QLatin1String("Proton"))
           || name.startsWith(QLatin1String("Steam Linux Runtime"))
           || name.startsWith(QLatin1String("Steamworks"));
}

ProviderScan SteamProvider::scan(const QString &steamRoot, GameRepository &repository) const
{
    ProviderScan result;
    for (const QString &steamapps : libraryFolders(steamRoot)) {
        const QDir dir(steamapps);
        for (const QFileInfo &info : dir.entryInfoList({QStringLiteral("appmanifest_*.acf")}, QDir::Files)) {
            QFile file(info.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly))
                continue;
            const QVariantMap app = Vdf::parse(file.readAll()).value(QStringLiteral("AppState")).toMap();
            const QString appId = app.value(QStringLiteral("appid")).toString();
            const QString name = app.value(QStringLiteral("name")).toString();
            const int flags = app.value(QStringLiteral("StateFlags")).toInt();

            // Bit 4 = instalado (pode estar com atualização pendente); ferramentas ficam de fora.
            if (appId.isEmpty() || !(flags & 4) || isTool(appId, name))
                continue;

            const ScannedFile manifest = fileFrom(info, QStringLiteral("steam"));
            result.seenPaths.insert(manifest.path);
            const QMap<QString, QString> artwork = localArtwork(steamRoot, appId);

            if (repository.isUnchanged(manifest.path, manifest.size, manifest.modified)) {
                ++result.unchanged;
                // A Steam pode ter baixado artes depois da última varredura.
                repository.updateMedia(QString::fromLatin1(Id), appId, artwork);
                continue;
            }

            ScannedGame game;
            game.platform = QStringLiteral("pc");
            game.provider = QString::fromLatin1(Id);
            game.externalId = appId;
            game.title = name.isEmpty() ? appId : name;
            game.files.append(manifest);
            game.media = artwork;
            qCInfo(lcScan) << "Steam:" << appId << game.title << "artes:" << artwork.keys();
            result.changed.append(game);
        }
    }
    return result;
}
