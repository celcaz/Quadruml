pragma Singleton
import QtQuick
import Quadrum

// Todos os valores visuais do app. Nenhuma tela usa número ou cor solta:
// tudo vem daqui, e daqui vem do theme.json do tema ativo (com padrões de reserva).
// Medidas em pixels na base 1920x1080; px() converte para a tela real.
QtObject {
    id: theme

    // A interface é desenhada em 1920x1080 e escalada pela altura da janela.
    property real scale: 1
    function px(value) { return Math.round(value * scale) }

    // Lê "a.b.c" do theme.json; devolve fallback se não existir.
    function token(path, fallback) {
        let node = ThemeService.tokens
        for (const key of path.split(".")) {
            if (node === undefined || node === null)
                return fallback
            node = node[key]
        }
        return node === undefined || node === null ? fallback : node
    }

    // Arquivo do tema (vazio se não existir). Ler "revision" recalcula quando o tema muda.
    function image(key) { return (ThemeService.revision, ThemeService.image(key)) }

    // Cores
    readonly property color background: token("colors.background", "#0a0d14")
    readonly property color backgroundAlt: token("colors.backgroundAlt", "#141a26")
    readonly property color text: token("colors.text", "#ffffff")
    readonly property color textMuted: token("colors.textMuted", "#9aa3b2")
    readonly property color focus: token("colors.focus", "#ffffff")
    readonly property color accent: token("colors.accent", "#0070d1")
    readonly property color surface: token("colors.surface", "#73101418")
    readonly property color button: token("colors.button", "#40ffffff")
    readonly property color buttonFocused: token("colors.buttonFocused", "#ffffff")
    readonly property color buttonTextFocused: token("colors.buttonTextFocused", "#000000")
    readonly property color scrim: token("colors.scrim", "#cc05070b")

    // Tipografia
    readonly property string fontFamily: ThemeService.fontFamily.length > 0 ? ThemeService.fontFamily : "Segoe UI"
    readonly property int fontHuge: px(token("type.huge", 64))
    readonly property int fontTitle: px(token("type.title", 44))
    readonly property int fontHeading: px(token("type.heading", 30))
    readonly property int fontBody: px(token("type.body", 26))
    readonly property int fontSmall: px(token("type.small", 20))

    // Formas
    readonly property int radiusSmall: px(token("shape.radiusSmall", 10))
    readonly property int radiusMedium: px(token("shape.radiusMedium", 16))
    readonly property int radiusLarge: px(token("shape.radiusLarge", 24))
    readonly property int focusRingWidth: px(token("shape.focusRingWidth", 4))
    readonly property int focusRingGap: px(token("shape.focusRingGap", 5))
    readonly property real blur: token("shape.blur", 0.8)

    // Movimento (milissegundos)
    readonly property int durationFast: token("motion.fast", 120)
    readonly property int durationNormal: token("motion.normal", 220)
    readonly property int durationSlow: token("motion.slow", 400)
    readonly property int durationBackground: token("motion.background", 600)

    // Layout geral
    readonly property int marginH: px(token("layout.marginH", 96))
    readonly property int marginTop: px(token("layout.marginTop", 40))

    // Tela inicial
    readonly property QtObject home: QtObject {
        readonly property int rowTop: theme.px(theme.token("home.rowTop", 120))
        readonly property int tileSize: theme.px(theme.token("home.tileSize", 136))
        readonly property int tileFocusedSize: theme.px(theme.token("home.tileFocusedSize", 196))
        readonly property int tileSpacing: theme.px(theme.token("home.tileSpacing", 18))
        readonly property int labelGap: theme.px(theme.token("home.labelGap", 28))
        readonly property int labelWidth: theme.px(theme.token("home.labelWidth", 420))
        readonly property int logoTop: theme.px(theme.token("home.logoTop", 400))
        readonly property int logoMaxWidth: theme.px(theme.token("home.logoMaxWidth", 680))
        readonly property int logoMaxHeight: theme.px(theme.token("home.logoMaxHeight", 200))
        readonly property int actionsTop: theme.px(theme.token("home.actionsTop", 640))
        readonly property int buttonHeight: theme.px(theme.token("home.buttonHeight", 64))
        readonly property int buttonWidth: theme.px(theme.token("home.buttonWidth", 220))
        readonly property int cardsTop: theme.px(theme.token("home.cardsTop", 800))
        readonly property int cardWidth: theme.px(theme.token("home.cardWidth", 400))
        readonly property int cardHeight: theme.px(theme.token("home.cardHeight", 180))
        readonly property int cardSpacing: theme.px(theme.token("home.cardSpacing", 24))
    }

    // Recursos do tema
    readonly property url logo: image("logo")
}
