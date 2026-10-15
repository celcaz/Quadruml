#include "GamepadService.h"

#include <QGuiApplication>
#include <QJSEngine>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QWindow>

#include <SDL3/SDL.h>

Q_LOGGING_CATEGORY(lcInput, "quadrum.input")

namespace {

GamepadService *s_instance = nullptr;

// Repetição ao segurar uma direção: espera inicial, depois acelera.
constexpr qint64 RepeatDelayMs = 350;
constexpr qint64 RepeatSlowMs = 120;
constexpr qint64 RepeatFastMs = 60;
constexpr int RepeatsBeforeFast = 6;

// Analógico: aperta acima de 50% e solta abaixo de 35% (evita tremer na borda).
constexpr int StickPress = 16384;
constexpr int StickRelease = 11500;

SDL_Gamepad *pad(void *p) { return static_cast<SDL_Gamepad *>(p); }

QString typeName(SDL_GamepadType type)
{
    switch (type) {
    case SDL_GAMEPAD_TYPE_PS5: return QStringLiteral("ps5");
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS3: return QStringLiteral("ps4");
    case SDL_GAMEPAD_TYPE_XBOX360:
    case SDL_GAMEPAD_TYPE_XBOXONE: return QStringLiteral("xbox");
    default: return QStringLiteral("other");
    }
}

}

GamepadService::GamepadService(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(8); // ~120 leituras por segundo
    connect(&m_timer, &QTimer::timeout, this, &GamepadService::poll);
}

GamepadService::~GamepadService()
{
    closeGamepad();
    if (m_sdlReady)
        SDL_Quit();
    s_instance = nullptr;
}

GamepadService *GamepadService::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool GamepadService::start()
{
    // Continua recebendo o controle com a janela sem foco (botão PS durante o jogo, Fase 2).
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        qCWarning(lcInput) << "SDL não iniciou:" << SDL_GetError();
        return false;
    }
    m_sdlReady = true;
    m_clock.start();
    openFirstGamepad();
    m_timer.start();
    qCInfo(lcInput) << "leitura do controle iniciada";
    return true;
}

void GamepadService::openFirstGamepad()
{
    if (m_gamepad)
        return;
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids && count > 0) {
        if (SDL_Gamepad *opened = SDL_OpenGamepad(ids[0])) {
            m_gamepad = opened;
            m_gamepadId = ids[0];
            m_name = QString::fromUtf8(SDL_GetGamepadName(opened));
            m_type = typeName(SDL_GetGamepadType(opened));
            qCInfo(lcInput) << "controle conectado:" << m_name << "tipo" << m_type;
            emit connectionChanged();
        }
    }
    SDL_free(ids);
}

void GamepadService::closeGamepad()
{
    if (!m_gamepad)
        return;
    SDL_CloseGamepad(pad(m_gamepad));
    m_gamepad = nullptr;
    m_gamepadId = 0;
    m_name.clear();
    m_type.clear();
    m_directions = {};
    qCInfo(lcInput) << "controle desconectado";
    emit connectionChanged();
}

void GamepadService::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            openFirstGamepad();
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (event.gdevice.which == m_gamepadId) {
                closeGamepad();
                openFirstGamepad(); // outro controle ainda conectado?
            }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (event.gbutton.which == m_gamepadId)
                handleButton(event.gbutton.button, event.gbutton.down);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            if (event.gaxis.which == m_gamepadId)
                updateStick(event.gaxis.axis, event.gaxis.value);
            break;
        default:
            break;
        }
    }

    // Repetição das direções seguradas.
    const qint64 now = m_clock.elapsed();
    static constexpr int keys[] = {Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right};
    for (int d = 0; d < DirectionCount; ++d) {
        DirectionState &state = m_directions[d];
        if (state.held && now >= state.nextRepeatAt) {
            sendKey(keys[d], true);
            ++state.repeats;
            state.nextRepeatAt = now + (state.repeats > RepeatsBeforeFast ? RepeatFastMs : RepeatSlowMs);
        }
    }
}

void GamepadService::handleButton(int button, bool down)
{
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP: m_directions[Up].dpad = down; updateDirection(Up); return;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: m_directions[Down].dpad = down; updateDirection(Down); return;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: m_directions[Left].dpad = down; updateDirection(Left); return;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: m_directions[Right].dpad = down; updateDirection(Right); return;
    default: break;
    }

    if (!down)
        return;

    QString name;
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: name = QStringLiteral("cross"); sendKey(Qt::Key_Return); break;
    case SDL_GAMEPAD_BUTTON_EAST: name = QStringLiteral("circle"); sendKey(Qt::Key_Escape); break;
    case SDL_GAMEPAD_BUTTON_NORTH: name = QStringLiteral("triangle"); sendKey(Qt::Key_T); break;
    case SDL_GAMEPAD_BUTTON_WEST: name = QStringLiteral("square"); break;
    case SDL_GAMEPAD_BUTTON_START: name = QStringLiteral("options"); sendKey(Qt::Key_Menu); break;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: name = QStringLiteral("l1"); sendKey(Qt::Key_PageUp); break;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: name = QStringLiteral("r1"); sendKey(Qt::Key_PageDown); break;
    case SDL_GAMEPAD_BUTTON_GUIDE: name = QStringLiteral("ps"); emit psPressed(); break;
    case SDL_GAMEPAD_BUTTON_BACK: name = QStringLiteral("share"); emit sharePressed(); break;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD: name = QStringLiteral("touchpad"); break;
    default: return;
    }
    m_lastButton = name;
    emit buttonPressed(name);
}

void GamepadService::updateStick(int axis, int value)
{
    auto set = [this](Direction direction, bool pressed) {
        m_directions[direction].stick = pressed;
        updateDirection(direction);
    };
    auto pressedFor = [](bool wasPressed, int v) { return wasPressed ? v > StickRelease : v > StickPress; };

    if (axis == SDL_GAMEPAD_AXIS_LEFTX) {
        set(Left, pressedFor(m_directions[Left].stick, -value));
        set(Right, pressedFor(m_directions[Right].stick, value));
    } else if (axis == SDL_GAMEPAD_AXIS_LEFTY) {
        set(Up, pressedFor(m_directions[Up].stick, -value));
        set(Down, pressedFor(m_directions[Down].stick, value));
    }
}

void GamepadService::updateDirection(Direction direction)
{
    static constexpr int keys[] = {Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right};
    static const char *names[] = {"up", "down", "left", "right"};

    DirectionState &state = m_directions[direction];
    const bool held = state.dpad || state.stick;
    if (held == state.held)
        return;
    state.held = held;
    if (held) {
        sendKey(keys[direction]);
        state.repeats = 0;
        state.nextRepeatAt = m_clock.elapsed() + RepeatDelayMs;
        m_lastButton = QString::fromLatin1(names[direction]);
        emit buttonPressed(m_lastButton);
    }
}

void GamepadService::sendKey(int key, bool autoRepeat)
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window)
        return; // app sem foco: as teclas não vão para o jogo, só os sinais PS/Share
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, QString(), autoRepeat);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, QString(), autoRepeat);
    QCoreApplication::sendEvent(window, &release);
}
