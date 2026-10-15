import QtQuick
import Quadrum

// Fileira de jogos da tela inicial, no comportamento do PS5: o jogo em foco fica
// sempre na primeira posição, os anteriores saem pela esquerda e cada capa
// calcula a própria posição a partir do índice em foco (sem sobreposição).
FocusScope {
    id: gameRow

    property alias model: repeater.model
    property int currentIndex: 0
    readonly property int count: repeater.count
    // Atualizado à mão: repeater.itemAt() não avisa quando os itens são recriados
    // (ex.: lista recarregada depois da varredura).
    property Item currentItem: null

    signal activated(int index)

    function refreshCurrent() {
        if (currentIndex >= count)
            currentIndex = Math.max(0, count - 1)
        currentItem = count > 0 ? repeater.itemAt(currentIndex) : null
    }

    onCurrentIndexChanged: refreshCurrent()
    onCountChanged: Qt.callLater(refreshCurrent)

    Keys.onLeftPressed: event => {
        if (currentIndex > 0) {
            currentIndex--
            Sounds.play("move")
        }
    }
    Keys.onRightPressed: event => {
        if (currentIndex < count - 1) {
            currentIndex++
            Sounds.play("move")
        }
    }
    Keys.onReturnPressed: {
        Sounds.play("confirm")
        gameRow.activated(currentIndex)
    }

    Repeater {
        id: repeater
        delegate: Tile { row: gameRow }
        onItemAdded: Qt.callLater(gameRow.refreshCurrent)
        onItemRemoved: Qt.callLater(gameRow.refreshCurrent)
    }
}
