import QtQuick
import Quadrum

// Tela inicial (wireframe 2). Três zonas de foco, de cima para baixo:
// fileira de jogos -> botões (Jogar, •••) -> cartões.
FocusScope {
    id: home

    signal openLibrary()

    readonly property Item current: row.currentItem
    readonly property bool currentIsGame: current !== null && current.kind === "game"

    function formatPlaytime(minutes) {
        if (minutes < 60)
            return qsTr("%n minuto(s) jogado(s)", "", minutes)
        return qsTr("%n hora(s) jogada(s)", "", Math.floor(minutes / 60))
    }

    BackgroundLayer {
        id: background

        anchors.fill: parent
        source: home.currentIsGame ? home.current.heroUrl : ""
        tint: home.currentIsGame ? home.current.accent : Theme.backgroundAlt
    }

    // Barra superior
    Row {
        x: Theme.marginH
        y: Theme.marginTop
        spacing: Theme.px(40)

        Text {
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontHeading
            font.weight: Font.DemiBold
            text: qsTr("Jogos")
        }
        Text {
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontHeading
            text: qsTr("Galeria")
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.marginH
        y: Theme.marginTop
        spacing: Theme.px(28)

        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.text
            font.pixelSize: Theme.fontHeading
            text: "⌕"
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.text
            font.pixelSize: Theme.fontHeading
            text: "⚙"
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.px(44)
            height: width
            radius: width / 2
            color: Theme.accent

            Text {
                anchors.centerIn: parent
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                text: ProfileService.currentName.length > 0 ? ProfileService.currentName[0].toUpperCase() : "?"
            }
        }
        Clock {
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Fileira de jogos: o item em foco fica sempre na primeira posição, à esquerda.
    GameRow {
        id: row

        x: Theme.marginH
        y: Theme.home.rowTop
        width: parent.width - x
        height: Theme.home.tileFocusedSize + Theme.px(56)
        model: LibraryService.recent
        focus: true

        KeyNavigation.down: home.currentIsGame ? actions : null

        onActivated: {
            if (home.currentIsGame)
                toast.show(qsTr("Abrir jogos chega na Fase 2"))
            else
                home.openLibrary()
        }
    }

    // Logo do jogo (ou o título, enquanto não houver logo)
    Item {
        x: Theme.marginH
        y: Theme.home.logoTop
        width: Theme.home.logoMaxWidth
        height: Theme.home.logoMaxHeight
        visible: home.currentIsGame

        Image {
            id: logo
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.height
            source: home.currentIsGame ? home.current.logoUrl : ""
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            verticalAlignment: Image.AlignBottom
            sourceSize.width: width
            sourceSize.height: height
        }

        Text {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: parent.width
            visible: logo.status !== Image.Ready
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontHuge
            font.weight: Font.Bold
            text: home.currentIsGame ? home.current.title : ""
        }
    }

    // Botões
    FocusScope {
        id: actions

        x: Theme.marginH
        y: Theme.home.actionsTop
        width: childrenRect.width
        height: Theme.home.buttonHeight
        visible: home.currentIsGame

        KeyNavigation.up: row
        KeyNavigation.down: cards
        Keys.onEscapePressed: {
            Sounds.play("back")
            row.forceActiveFocus()
        }

        Row {
            spacing: Theme.px(20)

            PillButton {
                id: play
                focus: true
                text: qsTr("Jogar")
                available: false
                KeyNavigation.right: more
                onActivated: toast.show(qsTr("Abrir jogos chega na Fase 2"))
            }
            PillButton {
                id: more
                width: height
                text: "•••"
                KeyNavigation.left: play
                onActivated: toast.show(qsTr("Opções do jogo chegam na Etapa 8"))
            }
        }
    }

    Text {
        x: Theme.marginH
        y: Theme.home.actionsTop + Theme.home.buttonHeight + Theme.px(20)
        visible: home.currentIsGame
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        text: home.currentIsGame
              ? home.current.platformLabel + "  ·  " + home.formatPlaytime(home.current.minutesPlayed)
              : ""
    }

    // Cartões
    FocusScope {
        id: cards

        x: Theme.marginH
        y: Theme.home.cardsTop
        width: cardRow.width
        height: Theme.home.cardHeight
        visible: home.currentIsGame

        KeyNavigation.up: actions
        Keys.onEscapePressed: {
            Sounds.play("back")
            row.forceActiveFocus()
        }

        Row {
            id: cardRow
            spacing: Theme.home.cardSpacing

            Card {
                id: aboutCard
                focus: true
                backdrop: background
                title: qsTr("Sobre o jogo")
                body: home.currentIsGame ? home.current.description : ""
                KeyNavigation.right: trophiesCard
            }
            Card {
                id: trophiesCard
                backdrop: background
                available: false
                title: qsTr("Troféus")
                body: qsTr("Disponível na Fase 3")
                KeyNavigation.left: aboutCard
                KeyNavigation.right: cloudCard
            }
            Card {
                id: cloudCard
                backdrop: background
                available: false
                title: qsTr("Save na nuvem")
                body: qsTr("Disponível na Fase 4")
                KeyNavigation.left: trophiesCard
            }
        }

        onActiveFocusChanged: if (activeFocus) Sounds.play("move")
    }

    HintBar {
        anchors.right: parent.right
        anchors.rightMargin: Theme.marginH
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(36)
        hints: [
            { button: "cross", text: qsTr("Selecionar") },
            { button: "circle", text: qsTr("Voltar") },
            { button: "options", text: qsTr("Opções") }
        ]
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(110)
    }
}
