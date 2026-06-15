/**
 * @file highscores.cpp
 * @brief Gestion complète des meilleurs scores pour AkaPong.
 *
 * Ce module gère :
 *   - La création automatique du fichier de scores sur la carte SD.
 *   - Le chargement et l’enregistrement des scores dans /sdcard/AkaPong/AkaPong.sco.
 *   - L’ajout d’un nouveau score avec saisie interactive du nom (jusqu’à 8 caractères).
 *   - Le tri décroissant des scores et la limitation au top MAX_SCORES.
 *   - L’affichage formaté du tableau des scores.
 *
 * Architecture :
 *   - Les scores sont stockés dans un fichier binaire contenant un tableau
 *     de structures HighscoreEntry (nom + score).
 *   - La saisie du nom utilise l’API input_poll() et fournit un retour audio.
 *   - L’affichage utilise gfx_text() et gfx_flush() pour un rendu immédiat.
 *
 * Notes :
 *   - Le fichier est stocké dans /sdcard/AkaPong/AkaPong.sco.
 *   - Le module est totalement indépendant du moteur du jeu.
 *   - highscores_submit() est appelé automatiquement en fin de partie.
 */

#include "highscores.h"
#include "core/graphics.h"
#include "core/input.h"
#include "lib/LCD.h"
#include "lib/graphics_basic.h"
#include "core/audio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#define HS_PATH "/sdcard/AkaPong/AkaPong.sco"

void highscores_init() {
    FILE* f = fopen(HS_PATH, "rb");
    if (!f) {
        f = fopen(HS_PATH, "wb");
        if (f) fclose(f);
    } else {
        fclose(f);
    }
}

std::vector<HighscoreEntry> highscores_load() {
    std::vector<HighscoreEntry> scores;
    FILE* f = fopen(HS_PATH, "rb");
    if (!f) return scores;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    size_t count = (size >= 0) ? (size_t)(size / sizeof(HighscoreEntry)) : 0;
    scores.resize(count);

    if (count > 0) {
        fread(scores.data(), sizeof(HighscoreEntry), count, f);
    }
    fclose(f);
    return scores;
}

void highscores_submit(int score) {
    auto scores = highscores_load();

    std::string name = highscores_input_name();

    HighscoreEntry entry;
    strncpy(entry.name, name.c_str(), 8);
    entry.name[8] = '\0';
    entry.score = score;

    scores.push_back(entry);

    std::sort(scores.begin(), scores.end(),
              [](const HighscoreEntry& a, const HighscoreEntry& b){
                  return a.score > b.score;
              });

    if (scores.size() > MAX_SCORES) scores.resize(MAX_SCORES);

    FILE* f = fopen(HS_PATH, "wb");
    if (f) {
        fwrite(scores.data(), sizeof(HighscoreEntry), scores.size(), f);
        fclose(f);
    }
}

void highscores_show() {
    auto scores = highscores_load();
    gfx_clear(color_black);
    gfx_text(80, 10, "=== Highscores ===", color_yellow);

    int y = 50;
    int rank = 1;
    for (const auto& e : scores) {
        if (rank > MAX_SCORES) break;
        if (e.name[0] == '\0') continue;

        char buf[64];
        snprintf(buf, sizeof(buf), "%2d. %-8s %6d", rank, e.name, e.score);
        gfx_text(20, y, buf, color_white);
        y += 20;
        rank++;
    }

    gfx_text(20, 170, "Appuyez sur <B> pour revenir.", color_yellow);
    gfx_flush();
}

std::string highscores_input_name() {
    std::string name;
    char currentChar = 'A';
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    int index = 0;

    bool b_pressed = false;

    while (true) {
        Keys k;
        input_poll(k);

        if (k.left) {
            if (--index < 0) index = alphabet.size()-1;
            currentChar = alphabet[index];
            snd_keypress.play_tone(880.0f, 50);
        }
        if (k.right) {
            if (++index >= alphabet.size()) index = 0;
            currentChar = alphabet[index];
            snd_keypress.play_tone(880.0f, 50);
        }

        if (k.A && name.size() < 8) {
            name.push_back(currentChar);
            index = 0;
            currentChar = 'A';
            snd_keypress.play_tone(880.0f, 50);
        }

        if (k.C && !name.empty()) {
            name.pop_back();
            snd_delete.play_tone(110.0f, 80);
        }

        if (k.B && !name.empty()) {
            b_pressed = true;
        }
        if (!k.B && b_pressed) {
            snd_keypress.play_tone(660.0f, 120);
            break;
        }

        gfx_clear(color_black);
        gfx_text(20, 100, ("Pseudo: " + name + currentChar).c_str(), color_white);
        gfx_text(20, 120, "A=Valider, C=Effacer, B=OK", color_yellow);
        gfx_flush();

        player.pool();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return name;
}
