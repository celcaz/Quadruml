import QtQuick
import Quadrum

Text {
    id: clock

    property date now: new Date()

    color: Theme.text
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody
    text: Qt.formatTime(now, "HH:mm")

    Timer {
        // Acorda no início de cada minuto, não a cada segundo.
        interval: (60 - clock.now.getSeconds()) * 1000
        running: true
        repeat: true
        onTriggered: clock.now = new Date()
    }
}
