/**
 * @file title_screen.cpp
 * @brief Écran titre d’AkaPong : logo + menu principal navigable.
 *
 * Ce module gère :
 *   - L’affichage de l’image de titre (title_image_pixels).
 *   - L’affichage d’un menu vertical :
 *         1. Jouer
 *         2. Options
 *         3. Highscores
 *         4. Retour Loader
 *   - La navigation via UP / DOWN avec limiteur de répétition.
 *   - La validation via A (la logique de changement d’état est gérée dans app_main.cpp).
 *
 * Notes :
 *   - Le retour loader OTA est déclenché dans app_main.cpp lorsque l’utilisateur
 *     sélectionne l’entrée “Retour Loader”.
 *   - Ce fichier ne modifie pas l’état du jeu : il fournit uniquement l’UI et
 *     la sélection utilisateur.
 */

#include "title_screen.h"
#include "assets/title_image.h"
#include "graphics.h"
#include "input.h"
#include "expander.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// Curseur du menu
static int cursor = 0;

// Items du menu
static const char* items[] = {
    "Jouer",
    "Options",
    "Highscores",
    "Retour Loader"
};

static const int ITEM_COUNT = sizeof(items) / sizeof(items[0]);

// Limiteur de répétition
static const int REPEAT_DELAY = 180;   // ms avant répétition
static const int REPEAT_RATE  = 120;   // ms entre répétitions
static uint32_t lastMoveTime = 0;
static bool repeatActive = false;

void title_screen_show()
{
    Keys k;
    input_poll(k);

    uint32_t now = esp_timer_get_time() / 1000; // en ms

    bool up   = k.up;
    bool down = k.down;

    // --- Déplacement initial ---
    if ((up || down) && !repeatActive) {
        if (up) {
            cursor--;
            if (cursor < 0) cursor = ITEM_COUNT - 1;
        }
        if (down) {
            cursor++;
            if (cursor >= ITEM_COUNT) cursor = 0;
        }

        repeatActive = true;
        lastMoveTime = now;
    }

    // --- Répétition après délai ---
    if (repeatActive && (up || down)) {
        if (now - lastMoveTime >= REPEAT_DELAY) {
            if (now - lastMoveTime >= REPEAT_RATE) {
                lastMoveTime = now;

                if (up) {
                    cursor--;
                    if (cursor < 0) cursor = ITEM_COUNT - 1;
                }
                if (down) {
                    cursor++;
                    if (cursor >= ITEM_COUNT) cursor = 0;
                }
            }
        }
    }

    // --- Reset si aucune touche n’est maintenue ---
    if (!up && !down) {
        repeatActive = false;
    }

    // --- Affichage ---
    gfx_clear(COLOR_BLACK);

    // Logo / image de titre
    lcd_draw_bitmap(title_image_pixels, TITLE_IMAGE_WIDTH, TITLE_IMAGE_HEIGHT, 0, 0);

    // Menu
    int y = 150;
    for (int i = 0; i < ITEM_COUNT; i++) {
        uint16_t col = (i == cursor) ? COLOR_YELLOW : COLOR_WHITE;
        gfx_text(40, y, items[i], col);
        y += 20;
    }

    gfx_flush();
}

int title_screen_get_selection()
{
    Keys k;
    input_poll(k);

    if (k.A) return cursor;
    return -1;
}
