#include "ui_common.h"
#include "raycanvas.h"
#include "settings.h"
#include <string.h>
#include <stdio.h>

Rectangle UIGetTelaRect(void) {
    return RayCanvasGetRect(
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.5f, 0.5f },
        (Vector2){ 0.0f, 0.0f },
        (Vector2){ (float)UI_VIRTUAL_W, (float)UI_VIRTUAL_H }
    );
}

Rectangle UIParaTela(Rectangle r) {
    Rectangle tela = UIGetTelaRect();
    float escalaX = tela.width  / (float)UI_VIRTUAL_W;
    float escalaY = tela.height / (float)UI_VIRTUAL_H;
    return (Rectangle){
        tela.x + r.x * escalaX,
        tela.y + r.y * escalaY,
        r.width  * escalaX,
        r.height * escalaY
    };
}

Vector2 UIParaTelaPos(Vector2 p) {
    Rectangle tela = UIGetTelaRect();
    float escalaX = tela.width  / (float)UI_VIRTUAL_W;
    float escalaY = tela.height / (float)UI_VIRTUAL_H;
    return (Vector2){
        tela.x + p.x * escalaX,
        tela.y + p.y * escalaY
    };
}

Vector2 UIMouseVirtual(void) {
    Vector2 m = RayCanvasGetMousePosition();
    Rectangle tela = UIGetTelaRect();
    if (tela.width <= 0.0f || tela.height <= 0.0f) return (Vector2){ 0.0f, 0.0f };
    return (Vector2){
        (m.x - tela.x) * (float)UI_VIRTUAL_W / tela.width,
        (m.y - tela.y) * (float)UI_VIRTUAL_H / tela.height
    };
}

float UIGetEscala(void) {
    return UIGetTelaRect().width / (float)UI_VIRTUAL_W;
}

void UIDrawTextInArea(const char *texto, Rectangle area, int tamanhoFonte, Color cor) {
    if (!texto || !texto[0] || area.width <= 0.0f || area.height <= 0.0f) return;
    Rectangle destino = UIParaTela(area);
    float escala = destino.height / area.height;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;

    float margem = 6.0f * escala;
    float maxLargura = destino.width - margem * 2.0f;
    if (maxLargura < 10.0f) maxLargura = destino.width;

    int minTam = (int)(5.0f * escala);
    if (minTam < 1) minTam = 1;

    if (strchr(texto, '\n') != NULL) {
        char linhas[4][256];
        int numLinhas = 0;
        const char *p = texto;
        while (*p != '\0' && numLinhas < 4) {
            int i = 0;
            while (*p != '\0' && *p != '\n' && i < 255) {
                linhas[numLinhas][i++] = *p++;
            }
            linhas[numLinhas][i] = '\0';
            numLinhas++;
            if (*p == '\n') p++;
        }
        for (int l = 0; l < numLinhas; l++) {
            while (tam > minTam && MeasureText(linhas[l], tam) > maxLargura) {
                tam--;
            }
        }
        float linhaEspaco = 4.0f * escala;
        float alturaTotal = (float)numLinhas * (float)tam + (float)(numLinhas - 1) * linhaEspaco;
        float yBase = destino.y + (destino.height - alturaTotal) / 2.0f;
        for (int l = 0; l < numLinhas; l++) {
            int lw = MeasureText(linhas[l], tam);
            float lx = destino.x + (destino.width - (float)lw) / 2.0f;
            if (lx < destino.x + margem) lx = destino.x + margem;
            DrawText(linhas[l], (int)lx, (int)(yBase + (float)l * ((float)tam + linhaEspaco)), tam, cor);
        }
        return;
    }

    while (tam > minTam && MeasureText(texto, tam) > maxLargura) {
        tam--;
    }

    int larguraTexto = MeasureText(texto, tam);
    float x = destino.x + (destino.width - (float)larguraTexto) / 2.0f;
    if (x < destino.x + margem) x = destino.x + margem;
    float y = destino.y + (destino.height - (float)tam) / 2.0f;
    DrawText(texto, (int)x, (int)y, tam, cor);
}

void UIDrawTextLeftAligned(const char *texto, Vector2 pos, int tamanhoFonte, Color cor) {
    if (!texto || !texto[0]) return;
    Rectangle destino = UIParaTela((Rectangle){ pos.x, pos.y, 10.0f, 10.0f });
    float escala = destino.height / 10.0f;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;
    DrawText(texto, (int)destino.x, (int)destino.y, tam, cor);
}

void UIDrawTextWrappedLeftAligned(const char *texto, Vector2 pos, float maxLargura, int tamanhoFonte, Color cor) {
    if (!texto || !texto[0]) return;
    Rectangle rEscala = UIParaTela((Rectangle){ 0.0f, 0.0f, maxLargura, 10.0f });
    float maxScaledW = rEscala.width;
    float escala = rEscala.height / 10.0f;
    int tam = (int)(tamanhoFonte * escala);
    if (tam < 1) tam = 1;

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s", texto);

    char *savePtr = NULL;
    char *word = strtok_r(buffer, " ", &savePtr);
    char line[1024] = "";
    float yOffset = 0.0f;

    while (word != NULL) {
        char testLine[1024];
        if (line[0] != '\0') {
            snprintf(testLine, sizeof(testLine), "%.500s %.500s", line, word);
        } else {
            snprintf(testLine, sizeof(testLine), "%.1000s", word);
        }

        if ((float)MeasureText(testLine, tam) > maxScaledW && line[0] != '\0') {
            UIDrawTextLeftAligned(line, (Vector2){ pos.x, pos.y + yOffset }, tamanhoFonte, cor);
            snprintf(line, sizeof(line), "%s", word);
            yOffset += (float)(tamanhoFonte + 4);
        } else {
            snprintf(line, sizeof(line), "%s", testLine);
        }
        word = strtok_r(NULL, " ", &savePtr);
    }
    if (line[0] != '\0') {
        UIDrawTextLeftAligned(line, (Vector2){ pos.x, pos.y + yOffset }, tamanhoFonte, cor);
    }
}

bool UIRemoverAcentos(char *dest, int tam, const char *src) {
    if (!dest || !src || tam <= 0) return false;
    static const char TABELA[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    dest[0] = '\0';
    int j = 0;
    for (int i = 0; src[i] != '\0' && j < tam - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == 0xC3 && (unsigned char)src[i + 1] >= 0x80 && (unsigned char)src[i + 1] <= 0xBF) {
            dest[j++] = TABELA[(unsigned char)src[i + 1] - 0x80];
            i++;
        } else {
            dest[j++] = (char)c;
        }
    }
    dest[j] = '\0';
    return true;
}

Texture2D UICriarSilhuetaColorida(const char *caminho, Color corBase, unsigned char alphaMin, bool mipmaps) {
    if (!caminho) return (Texture2D){ 0 };
    Image img = LoadImage(caminho);
    if (img.data == NULL) return (Texture2D){ 0 };
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = (Color *)img.data;
    int total = img.width * img.height;
    for (int i = 0; i < total; i++) {
        if (px[i].a > alphaMin) {
            px[i] = (Color){ corBase.r, corBase.g, corBase.b, px[i].a };
        } else if (alphaMin > 0) {
            px[i].a = 0;
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (tex.id > 0) {
        if (mipmaps) {
            GenTextureMipmaps(&tex);
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        } else {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        }
    }
    return tex;
}

void UISafeUnloadTexture(Texture2D *tex) {
    if (tex && tex->id > 0) {
        UnloadTexture(*tex);
        *tex = (Texture2D){ 0 };
    }
}

void UISafeUnloadSound(Sound *snd) {
    if (snd && snd->frameCount > 0) {
        UnloadSound(*snd);
        *snd = (Sound){ 0 };
    }
}

void UISafeUnloadShader(Shader *shd) {
    if (shd && shd->id > 0) {
        UnloadShader(*shd);
        *shd = (Shader){ 0 };
    }
}

void UITocarSomHover(Sound snd) {
    if (snd.frameCount > 0) {
        SetSoundVolume(snd, 0.25f * SettingsGetSfxVolume());
        PlaySound(snd);
    }
}

void UIDrawCloseButton(Rectangle rectVirtual) {
    Rectangle r = UIParaTela(rectVirtual);
    bool hov = CheckCollisionPointRec(RayCanvasGetMousePosition(), r);
    Color corFundo = hov ? UI_COR_NEGACAO : UI_COR_NEGACAO_FUNDO;
    Color corBorda = hov ? (Color){ 255, 242, 230, 255 } : UI_COR_NEGACAO_CLARO;
    Color corX     = (Color){ 255, 242, 230, 255 };

    DrawRectangleRec(r, corFundo);
    DrawRectangleLinesEx(r, hov ? 1.5f : 1.0f, corBorda);

    float cx = r.x + r.width  * 0.5f;
    float cy = r.y + r.height * 0.5f;
    float lado = (r.width < r.height) ? r.width : r.height;
    float braco = lado * 0.27f;
    float espessura = lado * 0.12f;
    if (espessura < 1.8f) espessura = 1.8f;

    DrawLineEx((Vector2){ cx - braco, cy - braco }, (Vector2){ cx + braco, cy + braco }, espessura, corX);
    DrawLineEx((Vector2){ cx + braco, cy - braco }, (Vector2){ cx - braco, cy + braco }, espessura, corX);
}

static const char *UI_SHADER_FUNDO_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D noiseTex1;\n"
    "uniform sampler2D noiseTex2;\n"
    "uniform float time;\n"
    "out vec4 finalColor;\n"
    "float bayer4x4(vec2 p) {\n"
    "    int x = int(mod(p.x, 4.0));\n"
    "    int y = int(mod(p.y, 4.0));\n"
    "    int idx = y * 4 + x;\n"
    "    const int m[16] = int[16](\n"
    "         0,  8,  2, 10,\n"
    "        12,  4, 14,  6,\n"
    "         3, 11,  1,  9,\n"
    "        15,  7, 13,  5\n"
    "    );\n"
    "    return (float(m[idx]) / 15.0) - 0.5;\n"
    "}\n"
    "void main() {\n"
    "    vec2 px = floor(fragTexCoord * vec2(640.0, 360.0));\n"
    "    vec2 uv = px / vec2(640.0, 360.0);\n"
    "    vec2 drift1 = vec2(sin(time * 0.14) * 0.012, cos(time * 0.11) * 0.012);\n"
    "    vec2 drift2 = vec2(cos(time * 0.18) * 0.018, sin(time * 0.15) * 0.018);\n"
    "    float v1 = texture(noiseTex1, clamp(uv + drift1, 0.002, 0.998)).r;\n"
    "    float v2 = texture(noiseTex2, clamp(uv + drift2, 0.002, 0.998)).r;\n"
    "    float val = v1 * 0.65 + v2 * 0.35;\n"
    "    float onda = sin(val * 16.0 + time * 0.35);\n"
    "    float b = bayer4x4(px);\n"
    "    float dOnda = b * 0.28;\n"
    "    float dVal  = b * 0.18;\n"
    "    vec3 cPreto   = vec3(  0.0,   0.0,   0.0) / 255.0;\n"
    "    vec3 cEscuro  = vec3( 32.0,  29.0,  31.0) / 255.0;\n"
    "    vec3 cMedio   = vec3( 90.0,  83.0,  83.0) / 255.0;\n"
    "    vec3 cClaro   = vec3(125.0, 112.0, 113.0) / 255.0;\n"
    "    vec3 col = cEscuro;\n"
    "    if (val + dVal < 0.36 || onda + dOnda < -0.45) col = cPreto;\n"
    "    else if (onda + dOnda > 0.65) col = cClaro;\n"
    "    else if (onda + dOnda > 0.20) col = cMedio;\n"
    "    finalColor = vec4(col, 1.0) * fragColor;\n"
    "}\n";

bool UIBgShaderInit(UIBgShader *bg) {
    if (!bg) return false;
    memset(bg, 0, sizeof(*bg));

    // Gera Perlin noise em metade da resolucao (320x180) com filtro bilinear:
    // 4x mais rapido na CPU e 4x menos VRAM mantendo o mesmo visual organico
    Image n1 = GenImagePerlinNoise(UI_VIRTUAL_W / 2, UI_VIRTUAL_H / 2, 0, 0, 4.5f);
    Image n2 = GenImagePerlinNoise(UI_VIRTUAL_W / 2, UI_VIRTUAL_H / 2, 137, 293, 14.0f);
    bg->noise1 = LoadTextureFromImage(n1);
    bg->noise2 = LoadTextureFromImage(n2);
    UnloadImage(n1);
    UnloadImage(n2);
    SetTextureFilter(bg->noise1, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(bg->noise2, TEXTURE_FILTER_BILINEAR);

    Image br = GenImageColor(1, 1, WHITE);
    bg->branco = LoadTextureFromImage(br);
    UnloadImage(br);
    SetTextureFilter(bg->branco, TEXTURE_FILTER_POINT);

    bg->shader = LoadShaderFromMemory(NULL, UI_SHADER_FUNDO_FS);
    if (bg->shader.id > 0) {
        bg->timeLoc   = GetShaderLocation(bg->shader, "time");
        bg->noise1Loc = GetShaderLocation(bg->shader, "noiseTex1");
        bg->noise2Loc = GetShaderLocation(bg->shader, "noiseTex2");
    }

    bg->valido = (bg->noise1.id > 0 && bg->noise2.id > 0 && bg->branco.id > 0 && bg->shader.id > 0);
    if (!bg->valido) {
        UIBgShaderUnload(bg);
        return false;
    }
    return true;
}

void UIBgShaderDraw(const UIBgShader *bg, float time, Color tint) {
    if (!bg || !bg->valido) return;
    Rectangle tela = UIGetTelaRect();
    SetShaderValue(bg->shader, bg->timeLoc, &time, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(bg->shader);
        SetShaderValueTexture(bg->shader, bg->noise1Loc, bg->noise1);
        SetShaderValueTexture(bg->shader, bg->noise2Loc, bg->noise2);
        DrawTexturePro(bg->branco,
                       (Rectangle){ 0, 0, (float)bg->branco.width, (float)bg->branco.height },
                       tela, (Vector2){ 0, 0 }, 0.0f, tint);
    EndShaderMode();
}

void UIBgShaderUnload(UIBgShader *bg) {
    if (!bg) return;
    UISafeUnloadTexture(&bg->noise1);
    UISafeUnloadTexture(&bg->noise2);
    UISafeUnloadTexture(&bg->branco);
    UISafeUnloadShader(&bg->shader);
    bg->timeLoc = -1;
    bg->noise1Loc = -1;
    bg->noise2Loc = -1;
    bg->valido = false;
}
