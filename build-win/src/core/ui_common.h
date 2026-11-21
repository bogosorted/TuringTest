#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "raylib.h"
#include <stdbool.h>

#define UI_VIRTUAL_W 640
#define UI_VIRTUAL_H 360

// Coordenadas virtuais (640x360) <-> coordenadas reais de janela
Rectangle UIGetTelaRect(void);
Rectangle UIParaTela(Rectangle r);
Vector2   UIParaTelaPos(Vector2 p);
Vector2   UIMouseVirtual(void);
float     UIGetEscala(void);

// Desenho de texto escalado no espaco virtual
void UIDrawTextInArea(const char *texto, Rectangle area, int tamanhoFonte, Color cor);
void UIDrawTextLeftAligned(const char *texto, Vector2 pos, int tamanhoFonte, Color cor);
void UIDrawTextWrappedLeftAligned(const char *texto, Vector2 pos, float maxLargura, int tamanhoFonte, Color cor);

// Remove acentos UTF-8 comuns em portugues para renderizacao com fontes simples
bool UIRemoverAcentos(char *dest, int tam, const char *src);

// Cria silhueta solida a partir do canal alpha de uma imagem
Texture2D UICriarSilhuetaColorida(const char *caminho, Color corBase, unsigned char alphaMin, bool mipmaps);

// Descarregamento seguro (zera os structs apos liberar)
void UISafeUnloadTexture(Texture2D *tex);
void UISafeUnloadSound(Sound *snd);
void UISafeUnloadShader(Shader *shd);

// Toca som de hover respeitando o volume de SFX das configuracoes
void UITocarSomHover(Sound snd);

// Cores padrao de acao negativa / fechar / erro
#define UI_COR_NEGACAO       ((Color){ 196,  72,  72, 255 })
#define UI_COR_NEGACAO_FUNDO ((Color){ 138,  48,  50, 255 })
#define UI_COR_NEGACAO_CLARO ((Color){ 224, 108, 104, 255 })

// Desenha o botao quadrado [X] padronizado no espaco virtual
void UIDrawCloseButton(Rectangle rectVirtual);

// Fundo procedural animado com Perlin noise + Bayer dithering compartilhado entre cenas
typedef struct {
    Texture2D noise1;
    Texture2D noise2;
    Texture2D branco;
    Shader    shader;
    int       timeLoc;
    int       noise1Loc;
    int       noise2Loc;
    bool      valido;
} UIBgShader;

bool UIBgShaderInit(UIBgShader *bg);
void UIBgShaderDraw(const UIBgShader *bg, float time, Color tint);
void UIBgShaderUnload(UIBgShader *bg);

#endif // UI_COMMON_H
