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

    // "Shadow of Rome" -> "SR", "Skate 3" -> "S3", "skate." -> "S".
    // Prefere palavras com maiúscula ou número (pula "of", "the"); se não houver, usa todas.
    function initials(text) {
        const words = text.replace(/[™®©.:,!?'"-]/g, " ").split(" ").filter(w => w.length > 0)
        const significant = words.filter(w => w[0] !== w[0].toLowerCase() || (w[0] >= "0" && w[0] <= "9"))
        return (significant.length > 0 ? significant : words).slice(0, 2).map(w => w[0].toUpperCase()).join("")
    }

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
            text: cover.initials(cover.title)
        }
    }

    Image {
        id: image

        anchors.fill: parent
        source: cover.source
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        // Capas verticais quase sempre têm o nome do jogo no alto: o recorte
        // quadrado fica com a parte de cima em vez do meio.
        verticalAlignment: implicitHeight > implicitWidth * 1.2 ? Image.AlignTop : Image.AlignVCenter
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
