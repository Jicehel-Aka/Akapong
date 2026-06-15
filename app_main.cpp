/**
 * @file app_main.cpp
 * @brief Point d'entrée principal du jeu AkaPong sur console AKA (ESP32-S3).
 *
 * Rôle :
 *   - Initialisation du matériel (LCD, PWM, ADC, expander, SD, audio).
 *   - Initialisation des couches core (input, graphics, audio, persist).
 *   - Chargement des assets et du système de highscores.
 *   - Gestion de la machine à états du jeu.
 *   - Gestion du retour au loader OTA (combo RUN+MENU ou via menu titre).
 *
 * Notes :
 *   - Le menu de l’écran titre est géré dans title_screen.cpp.
 *   - highscores.cpp gère la sauvegarde dans /sdcard/AkaPong/AkaPong.sco.
 *   - Toute nouvelle fonctionnalité globale doit passer par cette machine à états.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_timer.h"

#include "driver/gpio.h"
#include "common.h"

// OTA loader
#include <esp_ota_ops.h>
#include <esp_partition.h>

// Librairies matérielles
#include "lib/expander.h"
#include "lib/LCD.h"
#include "lib/graphics_basic.h"
#include "lib/audio.h"
#include "lib/audio_basic.h"
#include "lib/sdcard.h"

// Couches core
#include "core/input.h"
#include "core/graphics.h"
#include "core/audio.h"
#include "core/persist.h"

// Composants du jeu
#include "game/config.h"
#include "ui/title_screen.h"
#include "ui/menu.h"
#include "ui/highscores.h"
#include "ui/hud.h"
#include "game/game.h"
#include "game/level_editor.h"
#include "assets/assets.h"

// Volume global
int volume = 128;

// Timer combo retour loader
static uint32_t combo_start = 0;

/**
 * @brief Retourne au loader OTA si RUN + MENU sont maintenus 500 ms.
 */
static void checkReturnToLoader()
{
    uint16_t raw = expander_read();

    bool run_held  = raw & EXPANDER_KEY_RUN;
    bool menu_held = raw & EXPANDER_KEY_MENU;

    if (run_held && menu_held) {
        uint32_t now = esp_timer_get_time() / 1000; // en ms

        if (!combo_start) {
            combo_start = now;
        } else if (now - combo_start >= 500) {
            combo_start = 0;

            const esp_partition_t* loader = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP,
                ESP_PARTITION_SUBTYPE_APP_OTA_1,
                nullptr
            );

            if (loader) {
                esp_ota_set_boot_partition(loader);
                esp_restart();
            }
        }
    } else {
        combo_start = 0;
    }
}

extern "C" void app_main()
{
    // --- Initialisation matériel ---
    lcd_init_pwm();
    lcd_update_pwm(64);
    adc_init();
    expander_init();
    LCD_init();
    sd_init();

    // --- Audio ---
    audio_init();
    audio_game_init();
    audio_set_volume(volume);

    // --- Entrées & assets ---
    input_init();
    init_assets();

    // --- Highscores ---
    highscores_init();   // crée /sdcard/AkaPong/AkaPong.sco si absent

    // Petit jingle de démarrage
    snd_level_start.play_tone(440.0f, 250);

    // État global du jeu
    GameState g;
    g.state = GameState::State::Title;

    // --- Boucle principale ---
    while (true) {
        Keys k;
        input_poll(k);

        // Combo loader global
        checkReturnToLoader();

        // Mise à jour audio
        player.pool();

        if (debug) {
            gfx_text(10, 10, ("State=" + std::to_string((int)g.state)).c_str(), color_white);
        }

        switch (g.state) {

        // ------------------------------
        // ÉCRAN TITRE (menu complet)
        // ------------------------------
        case GameState::State::Title:
        {
            title_screen_show();
            int sel = title_screen_get_selection();

            if (sel == 0) { // Jouer
                snd_level_start.play_tone(523.0, 80);
                snd_level_start.play_tone(659.0, 80);
                snd_level_start.play_tone(784.0, 120);
                game_init(g);
                g.state = GameState::State::Playing;
            }
            else if (sel == 1) { // Options
                g.state = GameState::State::Options;
            }
            else if (sel == 2) { // Highscores
                g.state = GameState::State::Highscores;
            }
            else if (sel == 3) { // Retour Loader
                const esp_partition_t* loader = esp_partition_find_first(
                    ESP_PARTITION_TYPE_APP,
                    ESP_PARTITION_SUBTYPE_APP_OTA_1,
                    nullptr
                );
                if (loader) {
                    esp_ota_set_boot_partition(loader);
                    esp_restart();
                }
            }
        }
        break;

        // ------------------------------
        // JEU EN COURS
        // ------------------------------
        case GameState::State::Playing:
            game_update(g);
            game_draw(g);

            if (k.RUN) {
                g.state = GameState::State::Paused;
            }

            if (game_is_over(g)) {
                snd_gameover.play_tone(130.0, 400);
                highscores_submit(g.score);
                g.state = GameState::State::Highscores;
            }
            break;

        // ------------------------------
        // OPTIONS
        // ------------------------------
        case GameState::State::Options:
            gfx_clear(color_black);
            gfx_text(20, 100, ("Volume: " + std::to_string(volume)).c_str(), color_white);
            gfx_text(20, 120, "LEFT/RIGHT pour regler, B pour retour", color_yellow);
            gfx_flush();

            if (k.left && volume > 0) volume -= 8;
            if (k.right && volume < 255) volume += 8;
            audio_set_volume(volume);

            if (k.B) g.state = GameState::State::Title;
            break;

        // ------------------------------
        // PAUSE
        // ------------------------------
        case GameState::State::Paused:
            gfx_text(20, 160, "Pause - A pour reprendre", color_white);
            gfx_flush();

            if (k.A) g.state = GameState::State::Playing;

            if (isLongPress(k, EXPANDER_KEY_RUN)) {
                g.state = GameState::State::GameOver;
            }
            break;

        // ------------------------------
        // RELANCE DE BALLE
        // ------------------------------
        case GameState::State::WaitingBall:
            gfx_text(50, 160, "Appuyez sur A pour lancer", color_yellow);

            Keys k2;
            input_poll(k2);

            if (k2.A) {
                if (!g.balls.empty()) {
                    g.balls[0].active = true;
                    g.balls[0].vx = BALL_SPEED_INIT;
                    g.balls[0].vy = -BALL_SPEED_INIT;
                }

                snd_launch.play_tone(659.0, 120);

                if (g.paddle.sticky_timer == -1) {
                    g.paddle.bonus_flags &= ~BONUS_GLUE;
                    g.paddle.sticky_timer = 0;
                }

                g.state = GameState::State::Playing;
            }
            break;

        // ------------------------------
        // HIGHSCORES
        // ------------------------------
        case GameState::State::Highscores:
            highscores_show();
            if (k.B) g.state = GameState::State::Title;
            break;

        // ------------------------------
        // GAME OVER
        // ------------------------------
        case GameState::State::GameOver:
            gfx_text(20, 160, "Game Over - A pour recommencer", color_red);
            snd_gameover.play_tone(130.0, 400);

            if (k.A || k.B) g.state = GameState::State::Title;
            break;

        default:
            g.state = GameState::State::Title;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
