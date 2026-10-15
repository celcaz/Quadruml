import QtQuick
import Quadrum

// Item da GameRow. O jogo em foco cresce e mostra nome e plataforma ao lado;
// o último item ("library") abre a Biblioteca.
Item {
    id: tile

    required property Item row
    required property int index
    required property string kind
    required property string title
    required property string platformLabel
    required property url coverUrl
    required property url heroUrl
    required property url logoUrl
    required property color accent
    required property string description
    required property int minutesPlayed
    required property string genre
    required property string releaseYear

    // Distância em itens até o jogo em foco: negativa = já passou (à esquerda).
    readonly property int offset: index - row.currentIndex
    readonly property bool current: offset === 0
    readonly property bool focused: current && row.activeFocus
    readonly property int step: Theme.home.tileSize + Theme.home.tileSpacing
    readonly property int focusedBlock: Theme.home.tileFocusedSize + Theme.home.labelGap
                                        + Theme.home.labelWidth + Theme.home.tileSpacing

    x: offset < 0 ? offset * step
       : offset === 0 ? 0
       : focusedBlock + (offset - 1) * step
    width: current ? Theme.home.tileFocusedSize : Theme.home.tileSize
    height: row.height
    opacity: offset < 0 ? 0 : 1
    visible: opacity > 0

    // Só anima depois de criado: uma capa nova (lista recarregada) já nasce no lugar.
    property bool ready: false
    Component.onCompleted: ready = true

    Behavior on x { enabled: tile.ready; NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
    Behavior on width { enabled: tile.ready; NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
    Behavior on opacity { enabled: tile.ready; NumberAnimation { duration: Theme.durationNormal } }

    Item {
        id: art

        width: tile.width
        height: width
        anchors.verticalCenter: parent.verticalCenter

        CoverArt {
            anchors.fill: parent
            visible: tile.kind === "game"
            source: tile.coverUrl
            title: tile.title
            accent: tile.accent
        }

        // Item "Biblioteca"
        Rectangle {
            anchors.fill: parent
            visible: tile.kind === "library"
            radius: Theme.radiusMedium
            color: Theme.surface

            Grid {
                anchors.centerIn: parent
                columns: 2
                spacing: parent.width * 0.06

                Repeater {
                    model: 4
                    Rectangle {
                        width: art.width * 0.16
                        height: width
                        radius: width * 0.2
                        color: Theme.text
                    }
                }
            }
        }
    }

    FocusRing {
        target: art
        shown: tile.focused
    }

    Column {
        x: art.width + Theme.home.labelGap
        anchors.verticalCenter: art.verticalCenter
        width: Theme.home.labelWidth
        spacing: Theme.px(6)
        opacity: tile.current && tile.kind === "game" ? 1 : 0
        visible: opacity > 0

        Behavior on opacity { enabled: tile.ready; NumberAnimation { duration: Theme.durationNormal } }

        Text {
            width: parent.width
            elide: Text.ElideRight
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontHeading
            font.weight: Font.Medium
            text: tile.title
        }

        Text {
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: tile.platformLabel
        }
    }

    // Rótulo do item "Biblioteca" quando em foco
    Text {
        anchors.top: art.bottom
        anchors.topMargin: Theme.px(10)
        anchors.horizontalCenter: art.horizontalCenter
        visible: tile.kind === "library" && tile.current
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        text: qsTr("Biblioteca")
    }
}
