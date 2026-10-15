import QtQuick
import Quadrum

// Aviso curto que aparece e some sozinho. Uso: toast.show("texto")
Rectangle {
    id: toast

    function show(message) {
        label.text = message
        opacity = 1
        hideTimer.restart()
    }

    width: label.implicitWidth + Theme.px(48)
    height: Theme.px(64)
    radius: height / 2
    color: Theme.scrim
    opacity: 0
    visible: opacity > 0

    Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }

    Text {
        id: label
        anchors.centerIn: parent
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
    }

    Timer {
        id: hideTimer
        interval: 2500
        onTriggered: toast.opacity = 0
    }
}
