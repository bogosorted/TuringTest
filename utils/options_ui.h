#ifndef OPTIONS_UI_H
#define OPTIONS_UI_H

#include "raylib.h"
#include <stdbool.h>

// Controles de opcoes (volume da musica, volume dos efeitos, tela cheia e paleta) que
// aparecem no menu principal e no menu de pausa (ESC) da gameplay.
// Le e grava direto em utils/settings (salva no settings.cfg sozinho).
//
// Desenha quatro linhas, em pixels da tela virtual 640x360:
//   Musica      [=======|-----]  80%
//   Efeitos     [==========|--] 100%
//   Tela cheia  [x] Ligada
//   Paleta      [<]   Nome   [>]  3/6
//                    ■ ■ ■ ■ ■

typedef struct {
    float x;             // onde comecam os nomes (Musica, Efeitos, Paleta)
    float direita;       // onde termina a coluna dos valores (alinhada a direita)
    float yPrimeira;     // centro vertical da linha da Musica
    float passoY;        // distancia entre as linhas
    float larguraValor;  // espaco reservado para "100%" / "3/8"
    float gapNome;       // espaco entre o nome mais largo e os controles
} OptionsUiLayout;

typedef struct {
    Font  fonte;
    float tamFonte;          // tamanho em pixels virtuais
    float espacamento;       // espacamento entre letras (pixels virtuais)
    bool  sombraTexto;
    Color nome;              // "Musica", "Efeitos", "Paleta"
    Color valor;             // "80%", "3/8"
    Color valorAtivo;        // valor com o mouse em cima / arrastando
    Color trilhoBorda;
    Color trilhoFundo;
    Color preenchido;
    Color preenchidoAtivo;
    Color puxadorBorda;
    Color setaBorda, setaBordaHover;
    Color setaFundo, setaFundoHover;
    Color setaTexto;
    Color nomePaleta;
    Color amostraBorda;
} OptionsUiEstilo;

// Zera o estado (slider sendo arrastado, hover). Chamar ao abrir o painel.
void OptionsUiReset(void);

// Atualiza e desenha as quatro linhas. 'alpha' faz o fade; com 'interativo' false
// so desenha. 'tocarHover' (pode ser NULL) e chamado ao passar o mouse num botao
// e ao soltar o slider dos efeitos (amostra do volume novo).
// Devolve true se o mouse esta sobre algum controle ou arrastando um slider.
bool OptionsUiUpdateDraw(const OptionsUiLayout *lay, const OptionsUiEstilo *est,
                         float alpha, bool interativo, void (*tocarHover)(void));

// true enquanto um slider esta sendo arrastado
bool OptionsUiArrastando(void);

#endif
