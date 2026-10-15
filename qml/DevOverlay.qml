import QtQuick
import Quadrum

// Painel de desenvolvimento (F12): estado do tema, banco, perfis e controle.
Rectangle {
    id: overlay

    width: info.implicitWidth + Theme.px(40)
    height: info.implicitHeight + Theme.px(32)
    radius: Theme.radiusMedium
    color: Theme.scrim

    component Line: Text {
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
    }

    Column {
        id: info

        anchors.centerIn: parent
        spacing: Theme.px(6)

        Line {
            color: Theme.text
            text: Qt.application.displayName + " " + Qt.application.version + qsTr(" · desenvolvimento")
        }
        Line {
            text: ThemeService.found ? qsTr("Tema: %1").arg(ThemeService.name)
                                     : qsTr("Tema PS5 não encontrado em themes/ps5. Usando valores padrão.")
        }
        Line { text: qsTr("Idioma: português (Brasil)") }
        Line {
            text: qsTr("Etapa 2: banco de dados versão %1 · %n perfil(is)", "", ProfileService.count).arg(databaseVersion)
        }
        Line {
            text: GamepadService.connected
                  ? qsTr("Etapa 3: %n controle(s) · ativo: %1 (%2) · último botão: %3", "", GamepadService.count)
                        .arg(GamepadService.controllerName).arg(GamepadService.controllerType)
                        .arg(GamepadService.lastButton || "—")
                  : qsTr("Etapa 3: nenhum controle conectado")
        }
        Line {
            visible: LibraryService.usingSampleData
            text: qsTr("Etapa 4: usando jogos de exemplo")
        }
        Line {
            text: qsTr("F11: tela cheia · Ctrl+L: idioma · Ctrl+P: perfil de teste · F12: este painel · Ctrl+Q: sair")
        }
    }
}
