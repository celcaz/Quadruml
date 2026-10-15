import QtQuick
import Quadrum

// Arte de fundo em tela cheia, com troca suave quando o jogo em foco muda.
// Sem imagem, usa um degradê com a cor do jogo.
Item {
    id: layer

    property url source
    property color tint: Theme.backgroundAlt

    // Duas camadas: a nova aparece por cima da antiga.
    property bool aInFront: false

    onSourceChanged: Qt.callLater(swap)
    onTintChanged: Qt.callLater(swap)
    Component.onCompleted: swap()

    function swap() {
        // Compara com a camada que está NA FRENTE (a que o usuário vê). A de trás
        // pode guardar uma imagem antiga igual à pedida, mas não está visível.
        const front = aInFront ? slideA : slideB
        const back = aInFront ? slideB : slideA
        if (front.shown && front.imageSource.toString() === source.toString()
                && Qt.colorEqual(front.slideTint, tint))
            return
        back.imageSource = source
        back.slideTint = tint
        back.z = 1
        front.z = 0
        back.fadeIn()
        aInFront = !aInFront
    }

    component Slide: Item {
        id: slide

        property url imageSource
        property color slideTint: Theme.backgroundAlt
        property bool shown: false

        function fadeIn() {
            shown = true
            opacity = 0
            fade.restart()
        }

        anchors.fill: parent
        opacity: 0

        NumberAnimation {
            id: fade
            target: slide
            property: "opacity"
            to: 1
            duration: Theme.durationBackground
            easing.type: Easing.InOutQuad
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Qt.darker(slide.slideTint, 2.2) }
                GradientStop { position: 1; color: Qt.darker(slide.slideTint, 1.1) }
            }
        }

        Image {
            anchors.fill: parent
            source: slide.imageSource
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            sourceSize.width: width
            sourceSize.height: height
        }
    }

    Slide { id: slideA }
    Slide { id: slideB }

    // Escurece a esquerda e a parte de baixo para o texto ficar legível.
    Rectangle {
        anchors.fill: parent
        z: 2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0.75) }
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.15) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0) }
        }
    }
    Rectangle {
        anchors.fill: parent
        z: 2
        gradient: Gradient {
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.7) }
        }
    }
    // Topo levemente escuro: ícones e relógio legíveis mesmo sobre fundos claros.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height * 0.22
        z: 2
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0.55) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0) }
        }
    }
}
