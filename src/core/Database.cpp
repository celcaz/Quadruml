#include "Database.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>

#include <vector>

Q_LOGGING_CATEGORY(lcDb, "quadrum.db")

namespace {

struct Migration
{
    int version;
    std::vector<const char *> statements;
};

// Nunca altere uma migração já publicada: crie uma nova com o próximo número.
const std::vector<Migration> &migrations()
{
    static const std::vector<Migration> list{
    {1, {
        R"(CREATE TABLE perfis (
               id INTEGER PRIMARY KEY,
               nome TEXT NOT NULL,
               avatar TEXT,
               pin_hash TEXT,
               idioma TEXT,
               criado_em TEXT NOT NULL DEFAULT (datetime('now')))
        )",
        R"(CREATE TABLE jogos (
               id INTEGER PRIMARY KEY,
               plataforma TEXT NOT NULL,
               provedor TEXT NOT NULL,
               id_externo TEXT NOT NULL,
               titulo TEXT NOT NULL,
               titulo_ordenacao TEXT NOT NULL,
               identificacao_pendente INTEGER NOT NULL DEFAULT 0,
               adicionado_em TEXT NOT NULL DEFAULT (datetime('now')),
               atualizado_em TEXT NOT NULL DEFAULT (datetime('now')),
               UNIQUE (provedor, id_externo))
        )",
        R"(CREATE INDEX idx_jogos_plataforma_titulo ON jogos (plataforma, titulo_ordenacao))",
        R"(CREATE TABLE jogo_arquivos (
               id INTEGER PRIMARY KEY,
               jogo_id INTEGER NOT NULL REFERENCES jogos(id) ON DELETE CASCADE,
               caminho TEXT NOT NULL UNIQUE,
               formato TEXT,
               tamanho INTEGER,
               modificado_em TEXT,
               disco INTEGER NOT NULL DEFAULT 1)
        )",
        R"(CREATE TABLE jogo_midia (
               jogo_id INTEGER NOT NULL REFERENCES jogos(id) ON DELETE CASCADE,
               tipo TEXT NOT NULL,
               caminho_local TEXT NOT NULL,
               origem TEXT,
               escolhida_pelo_usuario INTEGER NOT NULL DEFAULT 0,
               PRIMARY KEY (jogo_id, tipo))
        )",
        R"(CREATE TABLE jogo_info (
               jogo_id INTEGER PRIMARY KEY REFERENCES jogos(id) ON DELETE CASCADE,
               descricao TEXT,
               genero TEXT,
               lancamento TEXT,
               fonte TEXT)
        )",
        R"(CREATE TABLE pastas (
               id INTEGER PRIMARY KEY,
               caminho TEXT NOT NULL UNIQUE,
               plataforma TEXT,
               padrao INTEGER NOT NULL DEFAULT 0)
        )",
        R"(CREATE TABLE config (
               chave TEXT PRIMARY KEY,
               valor TEXT)
        )",
    }},
    };
    return list;
}

}

Database::Database(QString path, QString connectionName)
    : m_path(std::move(path))
    , m_connectionName(std::move(connectionName))
{
}

Database::~Database()
{
    close();
}

bool Database::open()
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        qCCritical(lcDb) << "não foi possível abrir" << m_path << ":" << m_lastError;
        return false;
    }

    {
        // journal_mode devolve uma linha; finish() fecha o comando, senão o
        // SQLite recusa o COMMIT das migrações ("statements in progress").
        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
        pragma.finish();
    }

    if (!migrate())
        return false;

    qCInfo(lcDb) << "banco aberto:" << m_path << "versão" << schemaVersion();
    return true;
}

void Database::close()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase Database::handle() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

int Database::schemaVersion() const
{
    QSqlQuery query(handle());
    if (query.exec(QStringLiteral("SELECT MAX(versao) FROM schema_version")) && query.next())
        return query.value(0).toInt();
    return 0;
}

int Database::latestVersion()
{
    return migrations().back().version;
}

bool Database::migrate()
{
    QSqlDatabase db = handle();
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_version ("
            " versao INTEGER PRIMARY KEY,"
            " aplicada_em TEXT NOT NULL DEFAULT (datetime('now')))"))) {
        m_lastError = query.lastError().text();
        return false;
    }

    const int current = schemaVersion();
    for (const Migration &migration : migrations()) {
        if (migration.version <= current)
            continue;

        db.transaction();
        for (const char *statement : migration.statements) {
            if (!query.exec(QString::fromUtf8(statement))) {
                m_lastError = query.lastError().text();
                qCCritical(lcDb) << "migração" << migration.version << "falhou:" << m_lastError;
                db.rollback();
                return false;
            }
        }
        query.prepare(QStringLiteral("INSERT INTO schema_version (versao) VALUES (?)"));
        query.addBindValue(migration.version);
        const bool recorded = query.exec();
        query.finish();
        if (!recorded || !db.commit()) {
            m_lastError = recorded ? db.lastError().text() : query.lastError().text();
            qCCritical(lcDb) << "migração" << migration.version << "não foi gravada:" << m_lastError;
            db.rollback();
            return false;
        }
        qCInfo(lcDb) << "migração aplicada:" << migration.version;
    }
    return true;
}
