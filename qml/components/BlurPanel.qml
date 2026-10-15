import QtQuick
import QtQuick.Effects
import Quadrum

// Painel translúcido com o fundo desfocado atrás dele (vidro fosco, como no PS5).
// backdrop = o item de fundo a desfocar (normalmente o BackgroundLayer da tela).
Item {
    id: panel

    property Item backdrop
    property real radius: Theme.radiusMedium
    property color tint: Theme.surface

    ShaderEffectSource {
        id: captured

        anchors.fill: parent
        sourceItem: panel.backdrop
        sourceRect: panel.backdrop ? panel.mapToItem(panel.backdrop, 0, 0, panel.width, panel.height)
                                   : Qt.rect(0, 0, 0, 0)
        visible: false
    }

    MultiEffect {
        anchors.fill: parent
        source: captured
        visible: panel.backdrop !== null
        blurEnabled: true
        blur: Theme.blur
        blurMax: 48
        maskEnabled: true
        maskSource: mask
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

    Item {
        id: mask

        anchors.fill: parent
        layer.enabled: true
        visible: false

        Rectangle {
            anchors.fill: parent
            radius: panel.radius
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: panel.radius
        color: panel.tint
    }
}
