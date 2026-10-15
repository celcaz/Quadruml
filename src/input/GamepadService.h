#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <array>

class QQmlEngine;
class QJSEngine;

// Lê o controle (DualSense e outros) pelo SDL3 e transforma os botões em
// teclas para a janela do app: direcional/analógico -> setas, Cruz -> Enter,
// Círculo -> Esc, Opções -> Menu, L1/R1 -> Page Up/Down, Triângulo -> T.
// Assim as telas QML tratam controle e teclado do mesmo jeito.
// PS e Share não viram teclas: saem como sinais, porque valem até durante o jogo.
class GamepadService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY connectionChanged)
    Q_PROPERTY(QString controllerType READ controllerType NOTIFY connectionChanged)
    Q_PROPERTY(QString lastButton READ lastButton NOTIFY buttonPressed)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    explicit GamepadService(QObject *parent);
    ~GamepadService() override;

    static GamepadService *create(QQmlEngine *, QJSEngine *);

    bool start();

    bool connected() const { return m_gamepad != nullptr; }
    QString controllerName() const { return m_name; }
    // "ps5", "ps4", "xbox" ou "other": define quais ícones de botão mostrar.
    QString controllerType() const { return m_type; }
    QString lastButton() const { return m_lastButton; }

signals:
    void connectionChanged();
    void buttonPressed(const QString &button);
    void psPressed();
    void sharePressed();

private:
    enum Direction { Up, Down, Left, Right, DirectionCount };

    struct DirectionState
    {
        bool dpad = false;
        bool stick = false;
        bool held = false;
        qint64 nextRepeatAt = 0;
        int repeats = 0;
    };

    void poll();
    void openFirstGamepad();
    void closeGamepad();
    void handleButton(int button, bool down);
    void updateDirection(Direction direction);
    void updateStick(int axis, int value);
    void sendKey(int key, bool autoRepeat = false);

    void *m_gamepad = nullptr; // SDL_Gamepad*, escondido para não expor o SDL no header
    quint32 m_gamepadId = 0;
    QString m_name;
    QString m_type;
    QString m_lastButton;
    bool m_sdlReady = false;

    QTimer m_timer;
    QElapsedTimer m_clock;
    std::array<DirectionState, DirectionCount> m_directions{};
};
