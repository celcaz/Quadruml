pragma Singleton
import QtQuick
import QtMultimedia
import Quadrum

// Sons da interface, vindos do tema (WAV). Sem arquivo no tema, fica em silêncio.
QtObject {
    id: sounds

    property real volume: 0.7

    readonly property SoundEffect move: SoundEffect {
        source: (ThemeService.revision, ThemeService.sound("move"))
        volume: sounds.volume
    }
    readonly property SoundEffect confirm: SoundEffect {
        source: (ThemeService.revision, ThemeService.sound("confirm"))
        volume: sounds.volume
    }
    readonly property SoundEffect back: SoundEffect {
        source: (ThemeService.revision, ThemeService.sound("back"))
        volume: sounds.volume
    }

    function play(name) {
        const effect = sounds[name]
        if (effect && effect.source.toString() !== "")
            effect.play()
    }
}
