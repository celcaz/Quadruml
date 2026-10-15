#include "GamepadService.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QJSEngine>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QWindow>

#include <SDL3/SDL.h>

#include "core/Paths.h"
#include "platform/Processes.h"
#include "platform/Windowing.h"

Q_LOGGING_CATEGORY(lcInput, "quadrum.input")

namespace {

GamepadService *s_instance = nullptr;

// Repetição ao segurar uma direção: espera inicial, depois acelera.
constexpr qint64 RepeatDelayMs = 350;
constexpr qint64 RepeatSlowMs = 120;
constexpr qint64 RepeatFastMs = 60;
constexpr int RepeatsBeforeFast = 6;

// Analógicos e gatilhos: aperta acima de 50% e solta abaixo de 35% (evita tremer na borda).
constexpr int AxisPress = 16384;
constexpr int AxisRelease = 11500;

// Controle virtual que a Steam cria quando o Steam Input assume um controle.
constexpr Uint16 SteamVendor = 0x28DE;
constexpr Uint16 SteamVirtualProduct = 0x11FF;

constexpr int DirectionKeys[] = {Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right};
constexpr const char *DirectionNames[] = {"up", "down", "left", "right"};

SDL_Gamepad *sdl(void *p) { return static_cast<SDL_Gamepad *>(p); }

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

void loadMappings(const QString &path, const char *origin)
{
    if (path.isEmpty())
        return;
    const int added = SDL_AddGamepadMappingsFromFile(path.toUtf8().constData());
    if (added < 0)
        qCWarning(lcInput) << "mapeamentos" << origin << "não carregados:" << SDL_GetError();
    else
        qCInfo(lcInput) << "mapeamentos" << origin << "carregados:" << added;
}

}

GamepadService::GamepadService(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_pollTimer.setTimerType(Qt::PreciseTimer);
    m_pollTimer.setInterval(8); // ~120 leituras por segundo
    connect(&m_pollTimer, &QTimer::timeout, this, &GamepadService::poll);

    m_steamTimer.setInterval(3000);
    connect(&m_steamTimer, &QTimer::timeout, this, &GamepadService::checkSteam);
}

GamepadService::~GamepadService()
{
    for (auto &[id, device] : m_devices)
        SDL_CloseGamepad(sdl(device.gamepad));
    m_devices.clear();
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

    // Base da comunidade (SDL_GameControllerDB) para controles genéricos e menos comuns.
    // Um gamecontrollerdb.txt na pasta de dados do usuário, se existir, tem a palavra final.
    loadMappings(Paths::findBundledFile(QStringLiteral("gamecontrollerdb.txt"), QStringLiteral("input")),
                 "da comunidade");
    const QString userMappings = Paths::dataRoot() + QStringLiteral("/gamecontrollerdb.txt");
    if (QFileInfo::exists(userMappings))
        loadMappings(userMappings, "do usuário");

    m_clock.start();
    int total = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&total);
    for (int i = 0; i < total; ++i)
        addDevice(ids[i]);
    SDL_free(ids);

    m_pollTimer.start();
    m_steamTimer.start();
    checkSteam();
    qCInfo(lcInput) << "leitura do controle iniciada";
    return true;
}

int GamepadService::count() const
{
    int usable = 0;
    for (const auto &[id, device] : m_devices)
        usable += isUsable(device) ? 1 : 0;
    return usable;
}

QString GamepadService::controllerName() const
{
    const auto it = m_devices.find(m_activeId);
    return it != m_devices.end() ? it->second.name : QString();
}

QString GamepadService::controllerType() const
{
    const auto it = m_devices.find(m_activeId);
    return it != m_devices.end() ? it->second.type : QString();
}

void GamepadService::addDevice(quint32 id)
{
    if (m_devices.contains(id))
        return;
    SDL_Gamepad *opened = SDL_OpenGamepad(id);
    if (!opened) {
        qCWarning(lcInput) << "não foi possível abrir o controle" << id << ":" << SDL_GetError();
        return;
    }

    Device device;
    device.gamepad = opened;
    device.name = QString::fromUtf8(SDL_GetGamepadName(opened));
    device.type = typeName(SDL_GetGamepadType(opened));
    device.steamVirtual = SDL_GetGamepadVendor(opened) == SteamVendor
                          && SDL_GetGamepadProduct(opened) == SteamVirtualProduct;
    qCInfo(lcInput) << "controle conectado:" << device.name << "tipo" << device.type
                    << (device.steamVirtual ? "(virtual da Steam)" : "");
    m_devices.emplace(id, std::move(device));

    if (!m_activeId || !isUsable(m_devices.at(m_activeId)))
        setActive(id);
    emit connectionChanged();
    checkSteam();
}

void GamepadService::removeDevice(quint32 id)
{
    const auto it = m_devices.find(id);
    if (it == m_devices.end())
        return;
    qCInfo(lcInput) << "controle desconectado:" << it->second.name;
    SDL_CloseGamepad(sdl(it->second.gamepad));
    m_devices.erase(it);

    if (id == m_activeId) {
        m_activeId = 0;
        for (const auto &[otherId, device] : m_devices) {
            if (isUsable(device)) {
                m_activeId = otherId;
                break;
            }
        }
        emit activeChanged();
    }
    refreshAllDirections(); // o que estava segurado nesse controle é solto
    emit connectionChanged();
    checkSteam();
}

bool GamepadService::hasRealDevice() const
{
    for (const auto &[id, device] : m_devices) {
        if (!device.steamVirtual)
            return true;
    }
    return false;
}

bool GamepadService::isUsable(const Device &device) const
{
    // Com o controle de verdade visível, o virtual da Steam é ignorado:
    // senão cada botão chegaria duas vezes.
    return !(device.steamVirtual && hasRealDevice());
}

void GamepadService::setActive(quint32 id)
{
    if (id == m_activeId)
        return;
    m_activeId = id;
    emit activeChanged();
}

void GamepadService::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            addDevice(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            removeDevice(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            const auto it = m_devices.find(event.gbutton.which);
            if (it != m_devices.end() && isUsable(it->second))
                handleButton(it->second, event.gbutton.button, event.gbutton.down);
            if (event.gbutton.down && it != m_devices.end() && isUsable(it->second))
                setActive(event.gbutton.which);
            break;
        }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            const auto it = m_devices.find(event.gaxis.which);
            if (it != m_devices.end() && isUsable(it->second))
                handleAxis(it->second, event.gaxis.axis, event.gaxis.value);
            break;
        }
        default:
            break;
        }
    }

    // Repetição das direções seguradas.
    const qint64 now = m_clock.elapsed();
    for (int d = 0; d < DirectionCount; ++d) {
        RepeatState &state = m_repeat[d];
        if (state.held && now >= state.nextRepeatAt) {
            sendKey(DirectionKeys[d], true);
            ++state.repeats;
            state.nextRepeatAt = now + (state.repeats > RepeatsBeforeFast ? RepeatFastMs : RepeatSlowMs);
        }
    }
}

void GamepadService::handleButton(Device &device, int button, bool down)
{
    auto direction = [&](Direction d) {
        device.dpad[d] = down;
        refreshDirection(d);
    };
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP: direction(Up); return;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: direction(Down); return;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: direction(Left); return;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: direction(Right); return;
    default: break;
    }

    if (!down)
        return;

    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: sendKey(Qt::Key_Return); press(QStringLiteral("cross")); break;
    case SDL_GAMEPAD_BUTTON_EAST: sendKey(Qt::Key_Escape); press(QStringLiteral("circle")); break;
    case SDL_GAMEPAD_BUTTON_NORTH: sendKey(Qt::Key_T); press(QStringLiteral("triangle")); break;
    case SDL_GAMEPAD_BUTTON_WEST: press(QStringLiteral("square")); break;
    case SDL_GAMEPAD_BUTTON_START: sendKey(Qt::Key_Menu); press(QStringLiteral("options")); break;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: sendKey(Qt::Key_PageUp); press(QStringLiteral("l1")); break;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: sendKey(Qt::Key_PageDown); press(QStringLiteral("r1")); break;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK: press(QStringLiteral("l3")); break;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK: press(QStringLiteral("r3")); break;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD: press(QStringLiteral("touchpad")); break;
    case SDL_GAMEPAD_BUTTON_MISC1: press(QStringLiteral("mic")); break;
    case SDL_GAMEPAD_BUTTON_GUIDE: press(QStringLiteral("ps")); emit psPressed(); break;
    case SDL_GAMEPAD_BUTTON_BACK: press(QStringLiteral("share")); emit sharePressed(); break;
    default: break;
    }
}

void GamepadService::handleAxis(Device &device, int axis, int value)
{
    auto stick = [&](Direction d, int v) {
        const bool pressed = device.stick[d] ? v > AxisRelease : v > AxisPress;
        if (pressed != device.stick[d]) {
            device.stick[d] = pressed;
            refreshDirection(d);
        }
    };

    switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX: stick(Left, -value); stick(Right, value); break;
    case SDL_GAMEPAD_AXIS_LEFTY: stick(Up, -value); stick(Down, value); break;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: handleTrigger(device.l2, value, QStringLiteral("l2")); break;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: handleTrigger(device.r2, value, QStringLiteral("r2")); break;
    default: break;
    }
}

void GamepadService::handleTrigger(bool &pressed, int value, const QString &name)
{
    const bool now = pressed ? value > AxisRelease : value > AxisPress;
    if (now == pressed)
        return;
    pressed = now;
    if (pressed)
        press(name);
}

void GamepadService::refreshDirection(Direction direction)
{
    bool held = false;
    for (const auto &[id, device] : m_devices) {
        if (isUsable(device) && (device.dpad[direction] || device.stick[direction]))
            held = true;
    }

    RepeatState &state = m_repeat[direction];
    if (held == state.held)
        return;
    state.held = held;
    if (held) {
        sendKey(DirectionKeys[direction]);
        state.repeats = 0;
        state.nextRepeatAt = m_clock.elapsed() + RepeatDelayMs;
        press(QString::fromLatin1(DirectionNames[direction]));
    }
}

void GamepadService::refreshAllDirections()
{
    for (int d = 0; d < DirectionCount; ++d)
        refreshDirection(static_cast<Direction>(d));
}

void GamepadService::press(const QString &name)
{
    m_lastButton = name;
    emit buttonPressed(name);
}

void GamepadService::sendKey(int key, bool autoRepeat)
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window) {
        // A janela pode ter aberto sem foco (o Windows nem sempre deixa um app
        // novo vir para a frente). Enquanto não há jogo aberto, o controle vale
        // para a janela do hub mesmo assim, e pedimos para ela vir para a frente.
        // Na Fase 2, com um jogo aberto, as teclas não serão enviadas.
        for (QWindow *candidate : QGuiApplication::topLevelWindows()) {
            if (candidate->isVisible()) {
                Windowing::bringToFront(candidate);
                break;
            }
        }
        window = QGuiApplication::focusWindow();
        if (!window)
            return; // o Windows recusou o foco; a próxima tecla tenta de novo
    }
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, QString(), autoRepeat);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, QString(), autoRepeat);
    QCoreApplication::sendEvent(window, &release);
}

void GamepadService::checkSteam()
{
    const bool blocking = !hasRealDevice() && Processes::isRunning(QStringLiteral("steam.exe"));
    if (blocking == m_steamMayBeBlocking)
        return;
    m_steamMayBeBlocking = blocking;
    if (blocking)
        qCInfo(lcInput) << "Steam aberta sem controle de verdade visível: o Steam Input pode estar segurando o controle";
    emit steamMayBeBlockingChanged();
}
