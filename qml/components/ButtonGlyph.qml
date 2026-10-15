import QtQuick
import Quadrum

// Ícone de um botão do controle. Usa a imagem do tema (images.buttons.<nome>)
// quando existir; senão, um símbolo simples. Mostra os símbolos do PlayStation
// ou do Xbox conforme o controle em uso.
Item {
    id: glyph

    property string button // cross, circle, triangle, square, options, l1, r1, ps

    readonly property bool xbox: GamepadService.controllerType === "xbox"
    readonly property url themed: (ThemeService.revision, ThemeService.asset(
                                       Theme.token("images.buttons." + button, "")))

    implicitWidth: Theme.px(34)
    implicitHeight: Theme.px(34)

    readonly property var symbols: ({
        cross: xbox ? "A" : "✕",
        circle: xbox ? "B" : "○",
        triangle: xbox ? "Y" : "△",
        square: xbox ? "X" : "□",
        options: "☰",
        l1: xbox ? "LB" : "L1",
        r1: xbox ? "RB" : "R1",
        ps: xbox ? "⊗" : "PS"
    })

    Image {
        anchors.fill: parent
        source: glyph.themed
        visible: status === Image.Ready
        sourceSize.width: width
        sourceSize.height: height
    }

    Rectangle {
        anchors.fill: parent
        visible: glyph.themed.toString() === ""
        radius: width / 2
        color: "transparent"
        border.color: Theme.text
        border.width: Math.max(1, Theme.px(2))

        Text {
            anchors.centerIn: parent
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: parent.height * (text.length > 1 ? 0.38 : 0.55)
            font.weight: Font.DemiBold
            text: glyph.symbols[glyph.button] || "?"
        }
    }
}
