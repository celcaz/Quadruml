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
    Component.onCompleted: Theme.scale = height / 1080

    // Atalhos de desenvolvimento
    Shortcut {
        sequence: "F11"
        onActivated: root.visibility = root.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
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

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.backgroundAlt }
            GradientStop { position: 1; color: Theme.background }
        }
    }

    Text {
        id: clock

        property date now: new Date()

        anchors {
            top: parent.top
            right: parent.right
            topMargin: Theme.px(48)
            rightMargin: Theme.px(64)
        }
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
        text: Qt.formatTime(now, "HH:mm")

        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: clock.now = new Date()
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.px(24)

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: Theme.logo
            sourceSize.height: Theme.px(120)
            visible: status === Image.Ready
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
            text: Qt.application.displayName
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            text: qsTr("Etapa 1: fundação")
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: ThemeService.found ? qsTr("Tema: %1").arg(ThemeService.name)
                                     : qsTr("Tema PS5 não encontrado em themes/ps5. Usando valores padrão.")
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: qsTr("Idioma: português (Brasil)")
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: qsTr("Etapa 2: banco de dados versão %1 · %n perfil(is)", "", ProfileService.count).arg(databaseVersion)
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.px(16)

            Repeater {
                model: ProfileService.model

                Rectangle {
                    required property string name
                    required property bool hasPin

                    width: label.implicitWidth + Theme.px(32)
                    height: Theme.px(48)
                    radius: height / 2
                    color: Theme.backgroundAlt
                    border.color: Theme.textMuted
                    border.width: 1

                    Text {
                        id: label
                        anchors.centerIn: parent
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSmall
                        text: parent.name + (parent.hasPin ? " 🔒" : "")
                    }
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            text: GamepadService.connected
                  ? qsTr("Etapa 3: controle %1 (%2) · último botão: %3").arg(GamepadService.controllerName)
                        .arg(GamepadService.controllerType).arg(GamepadService.lastButton || "—")
                  : qsTr("Etapa 3: nenhum controle conectado")
        }

        // Teste de foco: navegue com o direcional ou as setas, confirme com Cruz ou Enter.
        ListView {
            id: focusTest

            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.px(6 * 160 + 5 * 24)
            height: Theme.px(160)
            orientation: ListView.Horizontal
            spacing: Theme.px(24)
            model: 6
            focus: true
            interactive: false
            keyNavigationWraps: false
            highlightMoveDuration: Theme.durationFast

            property int confirmed: -1

            Keys.onReturnPressed: confirmed = currentIndex

            delegate: Rectangle {
                required property int index

                width: Theme.px(160)
                height: width
                radius: Theme.px(12)
                color: Theme.backgroundAlt
                scale: ListView.isCurrentItem ? 1.08 : 1.0
                border.width: ListView.isCurrentItem ? Theme.px(4) : 0
                border.color: Theme.focus

                Behavior on scale { NumberAnimation { duration: Theme.durationFast; easing.type: Easing.OutCubic } }

                Text {
                    anchors.centerIn: parent
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    text: focusTest.confirmed === parent.index ? "✓" : parent.index + 1
                }
            }
        }
    }

    Text {
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: Theme.px(32)
            rightMargin: Theme.px(64)
        }
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        text: qsTr("F11: tela cheia · Ctrl+L: idioma · Ctrl+P: perfil de teste · Ctrl+Q: sair")
    }
}
