#pragma once

#include <QSqlDatabase>
#include <QString>

// Abre o hub.db (SQLite) e aplica as migrações pendentes.
// Cada migração tem um número; o banco guarda em schema_version quais já rodaram,
// então atualizar o app nunca apaga dados: só acrescenta o que falta.
class Database
{
public:
    explicit Database(QString path, QString connectionName = QStringLiteral("quadrum"));
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open();
    void close();

    QSqlDatabase handle() const;
    int schemaVersion() const;
    QString lastError() const { return m_lastError; }

    static int latestVersion();

private:
    bool migrate();

    QString m_path;
    QString m_connectionName;
    QString m_lastError;
};
