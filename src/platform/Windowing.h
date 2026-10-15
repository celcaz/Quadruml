#pragma once

class QWindow;

namespace Windowing {

// Traz a janela para a frente e dá o foco a ela. O Windows normalmente impede
// um app de "roubar" o foco; como o hub funciona como um console (o controle é
// a entrada principal), usamos o método de anexar a entrada à janela da frente.
void bringToFront(QWindow *window);

}
