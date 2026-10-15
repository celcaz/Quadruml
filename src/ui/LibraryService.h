#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "GameListModel.h"

class GameRepository;
struct ScanSummary;
class QQmlEngine;
class QJSEngine;

// Jogos para as telas. Mostra os jogos do banco; se ainda não houver nenhum,
// mostra uma lista de exemplo para a tela não ficar vazia.
class LibraryService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(GameListModel *recent READ recent CONSTANT)
    Q_PROPERTY(bool usingSampleData READ usingSampleData NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString scanSummary READ scanSummary NOTIFY changed)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    LibraryService(GameRepository *repository, QObject *parent);
    ~LibraryService() override;

    static LibraryService *create(QQmlEngine *, QJSEngine *);

    GameListModel *recent() { return &m_recent; }
    bool usingSampleData() const { return m_usingSampleData; }
    bool scanning() const { return m_scanning; }
    QString scanSummary() const { return m_scanSummary; }

    void reload();
    void setScanning(bool scanning);
    void setScanResult(const ScanSummary &summary);

signals:
    void changed();
    void scanningChanged();

private:
    void loadSampleData();

    GameRepository *m_repository;
    GameListModel m_recent{true};
    bool m_usingSampleData = false;
    bool m_scanning = false;
    QString m_scanSummary;
};
