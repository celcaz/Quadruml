import QtQuick
import Quadrum

// Item da fileira da tela inicial. O jogo em foco cresce e mostra nome e
// plataforma ao lado; o último item ("library") abre a Biblioteca.
Item {
    id: tile

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

    readonly property bool current: ListView.isCurrentItem
    readonly property bool focused: current && ListView.view.activeFocus
    readonly property int artSize: current ? Theme.home.tileFocusedSize : Theme.home.tileSize

    width: current && kind === "game"
           ? Theme.home.tileFocusedSize + Theme.home.labelGap + Theme.home.labelWidth
           : artSize
    height: ListView.view.height

    Behavior on width { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }

    Item {
        id: art

        width: tile.artSize
        height: width
        anchors.verticalCenter: parent.verticalCenter

        Behavior on width { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }

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
        anchors.left: art.right
        anchors.leftMargin: Theme.home.labelGap
        anchors.verticalCenter: art.verticalCenter
        width: Theme.home.labelWidth
        spacing: Theme.px(6)
        opacity: tile.current && tile.kind === "game" ? 1 : 0
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }

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
