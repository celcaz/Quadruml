import QtQuick
import QtQuick.Effects
import Quadrum

// Capa quadrada de um jogo, com cantos arredondados. Sem imagem, desenha um
// marcador neutro com a cor do jogo e as iniciais do título.
Item {
    id: cover

    property url source
    property string title
    property color accent: Theme.backgroundAlt
    property real radius: Theme.radiusMedium

    readonly property bool hasImage: image.status === Image.Ready

    Rectangle {
        anchors.fill: parent
        radius: cover.radius
        visible: !cover.hasImage
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.lighter(cover.accent, 1.35) }
            GradientStop { position: 1; color: Qt.darker(cover.accent, 1.6) }
        }

        Text {
            anchors.centerIn: parent
            color: Theme.text
            opacity: 0.9
            font.family: Theme.fontFamily
            font.pixelSize: parent.height * 0.3
            font.weight: Font.DemiBold
            text: cover.title.split(" ")
                       .filter(word => word.length > 0 && word[0] === word[0].toUpperCase())
                       .slice(0, 2).map(word => word[0]).join("")
        }
    }

    Image {
        id: image

        anchors.fill: parent
        source: cover.source
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: width
        sourceSize.height: height
        visible: false
    }

    MultiEffect {
        anchors.fill: parent
        source: image
        visible: cover.hasImage
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
            radius: cover.radius
        }
    }
}
