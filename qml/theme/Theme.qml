pragma Singleton
import QtQuick
import Quadrum

// Todos os valores visuais do app. Nenhuma tela usa número ou cor solta:
// tudo vem daqui, e daqui vem do theme.json do tema ativo (com padrões de reserva).
QtObject {
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

    // Cores
    readonly property color background: token("colors.background", "#0a0d14")
    readonly property color backgroundAlt: token("colors.backgroundAlt", "#141a26")
    readonly property color text: token("colors.text", "#ffffff")
    readonly property color textMuted: token("colors.textMuted", "#9aa3b2")
    readonly property color focus: token("colors.focus", "#ffffff")
    readonly property color accent: token("colors.accent", "#0070d1")

    // Tipografia (tamanhos na base 1080p)
    readonly property string fontFamily: ThemeService.fontFamily.length > 0 ? ThemeService.fontFamily : "Segoe UI"
    readonly property int fontTitle: px(token("type.title", 44))
    readonly property int fontBody: px(token("type.body", 26))
    readonly property int fontSmall: px(token("type.small", 20))

    // Movimento (milissegundos)
    readonly property int durationFast: token("motion.fast", 120)
    readonly property int durationNormal: token("motion.normal", 220)
    readonly property int durationSlow: token("motion.slow", 400)

    // Recursos do tema (URL vazia quando o arquivo não existe).
    // Ler "revision" faz o valor ser recalculado quando o tema muda.
    readonly property url logo: (ThemeService.revision, ThemeService.image("logo"))
}
