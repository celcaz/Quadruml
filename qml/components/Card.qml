import QtQuick
import Quadrum

// Cartão de informação sobre fundo desfocado. Recebe foco como os demais itens.
FocusScope {
    id: card

    property Item backdrop
    property string title
    property string body
    property bool available: true

    implicitWidth: Theme.home.cardWidth
    implicitHeight: Theme.home.cardHeight

    BlurPanel {
        id: panel

        anchors.fill: parent
        backdrop: card.backdrop
        opacity: card.available ? 1 : 0.6
    }

    FocusRing {
        target: panel
        shown: card.activeFocus
    }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.px(24)
        spacing: Theme.px(10)
        opacity: card.available ? 1 : 0.6

        Text {
            width: parent.width
            elide: Text.ElideRight
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.weight: Font.Medium
            text: card.title
        }

        Text {
            width: parent.width
            height: parent.height - y
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: card.body
        }
    }
}
