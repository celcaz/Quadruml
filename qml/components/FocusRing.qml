import QtQuick
import Quadrum

// Contorno branco de foco, por fora do item (como no PS5).
// Uso: FocusRing { target: algumItem; shown: algumItem.activeFocus; radius: ... }
Rectangle {
    id: ring

    property Item target: parent
    property bool shown: false
    property real targetRadius: Theme.radiusMedium

    x: target ? target.x - Theme.focusRingGap : 0
    y: target ? target.y - Theme.focusRingGap : 0
    width: target ? target.width + 2 * Theme.focusRingGap : 0
    height: target ? target.height + 2 * Theme.focusRingGap : 0
    radius: targetRadius + Theme.focusRingGap
    color: "transparent"
    border.color: Theme.focus
    border.width: Theme.focusRingWidth
    opacity: shown ? 1 : 0
    visible: opacity > 0

    Behavior on opacity { NumberAnimation { duration: Theme.durationFast } }
}
