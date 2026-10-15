#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <map>

class QQmlEngine;
class QJSEngine;

// Lê os controles (DualSense e outros) pelo SDL3 e transforma os botões em
// teclas para a janela do app: direcional/analógico -> setas, Cruz -> Enter,
// Círculo -> Esc, Opções -> Menu, L1/R1 -> Page Up/Down, Triângulo -> T.
// Assim as telas QML tratam controle e teclado do mesmo jeito.
// PS e Share não viram teclas: saem como sinais, porque valem até durante o jogo.
//
// Qualquer controle conectado navega. O controle "ativo" (o último usado)
// define nome e tipo, usados para mostrar os ícones de botão certos.
class GamepadService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(int count READ count NOTIFY connectionChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY activeChanged)
    Q_PROPERTY(QString controllerType READ controllerType NOTIFY activeChanged)
    Q_PROPERTY(QString lastButton READ lastButton NOTIFY buttonPressed)
    Q_PROPERTY(bool steamMayBeBlocking READ steamMayBeBlocking NOTIFY steamMayBeBlockingChanged)

public:
    // Sem construtor padrão de propósito (ver ThemeService).
    explicit GamepadService(QObject *parent);
    ~GamepadService() override;

    static GamepadService *create(QQmlEngine *, QJSEngine *);

    bool start();

    bool connected() const { return count() > 0; }
    int count() const;
    QString controllerName() const;
    // "ps5", "ps4", "xbox" ou "other".
    QString controllerType() const;
    QString lastButton() const { return m_lastButton; }
    // A Steam está aberta e só vemos o controle virtual dela (ou nenhum):
    // provavelmente o Steam Input está segurando o controle de verdade.
    bool steamMayBeBlocking() const { return m_steamMayBeBlocking; }

signals:
    void connectionChanged();
    void activeChanged();
    void buttonPressed(const QString &button);
    void psPressed();
    void sharePressed();
    void steamMayBeBlockingChanged();

private:
    enum Direction { Up, Down, Left, Right, DirectionCount };

    struct Device
    {
        void *gamepad = nullptr; // SDL_Gamepad*, escondido para não expor o SDL no header
        QString name;
        QString type;
        bool steamVirtual = false;
        std::array<bool, DirectionCount> dpad{};
        std::array<bool, DirectionCount> stick{};
        bool l2 = false;
        bool r2 = false;
    };

    struct RepeatState
    {
        bool held = false;
        qint64 nextRepeatAt = 0;
        int repeats = 0;
    };

    void poll();
    void addDevice(quint32 id);
    void removeDevice(quint32 id);
    bool isUsable(const Device &device) const;
    bool hasRealDevice() const;
    void setActive(quint32 id);
    void handleButton(Device &device, int button, bool down);
    void handleAxis(Device &device, int axis, int value);
    void handleTrigger(bool &pressed, int value, const QString &name);
    void refreshDirection(Direction direction);
    void refreshAllDirections();
    void press(const QString &name);
    void sendKey(int key, bool autoRepeat = false);
    void checkSteam();

    std::map<quint32, Device> m_devices;
    quint32 m_activeId = 0;
    QString m_lastButton;
    bool m_sdlReady = false;
    bool m_steamMayBeBlocking = false;

    QTimer m_pollTimer;
    QTimer m_steamTimer;
    QElapsedTimer m_clock;
    std::array<RepeatState, DirectionCount> m_repeat{};
};
