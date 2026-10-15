import QtQuick
import Quadrum

// Botão arredondado (Jogar, •••, abas). Em foco fica branco com texto escuro.
// "available: false" mostra o botão apagado, mas ele continua recebendo foco.
FocusScope {
    id: button

    property string text
    property bool available: true
    signal activated()

    implicitWidth: Theme.home.buttonWidth
    implicitHeight: Theme.home.buttonHeight

    Keys.onReturnPressed: {
        Sounds.play("confirm")
        button.activated()
    }

    Rectangle {
        id: body

        anchors.fill: parent
        radius: height / 2
        color: button.activeFocus ? Theme.buttonFocused : Theme.button
        opacity: button.available ? 1 : 0.45
        scale: button.activeFocus ? 1.04 : 1

        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        Behavior on scale { NumberAnimation { duration: Theme.durationFast; easing.type: Easing.OutCubic } }

        Text {
            anchors.centerIn: parent
            color: button.activeFocus ? Theme.buttonTextFocused : Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.weight: Font.Medium
            text: button.text
        }
    }
}
