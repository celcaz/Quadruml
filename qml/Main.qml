import QtQuick
import QtQuick.Window
import Quadrum

Window {
    id: root

    width: 1280
    height: 720
    visible: true
    visibility: Window.FullScreen
    title: Qt.application.displayName
    color: Theme.background

    onHeightChanged: Theme.scale = height / 1080
    Component.onCompleted: {
        Theme.scale = height / 1080
        home.forceActiveFocus()
    }

    // Atalhos de desenvolvimento
    Shortcut {
        sequence: "F11"
        onActivated: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
    }
    Shortcut {
        sequence: "F12"
        onActivated: devOverlay.visible = !devOverlay.visible
    }
    Shortcut {
        sequence: "Ctrl+L"
        onActivated: LanguageService.language = LanguageService.language === "pt_BR" ? "en" : "pt_BR"
    }
    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: ProfileService.createProfile(qsTr("Perfil %1").arg(ProfileService.count + 1))
    }

    Home {
        id: home
        anchors.fill: parent
        focus: true
    }

    // Aviso da Steam fica sempre visível, mesmo sem o painel de desenvolvimento.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(110)
        visible: GamepadService.steamMayBeBlocking
        width: steamText.implicitWidth + Theme.px(48)
        height: steamText.implicitHeight + Theme.px(28)
        radius: Theme.radiusMedium
        color: Theme.scrim

        Text {
            id: steamText
            anchors.centerIn: parent
            color: "#f2c94c"
            horizontalAlignment: Text.AlignHCenter
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: qsTr("A Steam pode estar usando o seu controle.\nFeche a Steam ou desative o Steam Input em Steam > Configurações > Controle.")
        }
    }

    DevOverlay {
        id: devOverlay
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Theme.px(24)
        visible: false
    }
}
