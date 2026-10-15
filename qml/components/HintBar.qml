import QtQuick
import Quadrum

// Rodapé com os botões disponíveis na tela. hints: [{ button: "cross", text: "..." }]
Row {
    id: bar

    property var hints: []

    spacing: Theme.px(36)

    Repeater {
        model: bar.hints

        Row {
            required property var modelData

            spacing: Theme.px(10)

            ButtonGlyph {
                anchors.verticalCenter: parent.verticalCenter
                button: modelData.button
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                text: modelData.text
            }
        }
    }
}
