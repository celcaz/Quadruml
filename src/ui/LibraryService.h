#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "GameListModel.h"

class QQmlEngine;
class QJSEngine;

// Jogos para as telas. Até a Etapa 7 (biblioteca real), usa uma lista de exemplo.
class LibraryService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(GameListModel *recent READ recent CONSTANT)
    Q_PROPERTY(bool usingSampleData READ usingSampleData NOTIFY changed)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    explicit LibraryService(QObject *parent);
    ~LibraryService() override;

    static LibraryService *create(QQmlEngine *, QJSEngine *);

    GameListModel *recent() { return &m_recent; }
    bool usingSampleData() const { return m_usingSampleData; }

    void loadSampleData();

signals:
    void changed();

private:
    GameListModel m_recent{true};
    bool m_usingSampleData = false;
};
