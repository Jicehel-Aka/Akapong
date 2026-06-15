/**
 * @file title_screen.h
 * @brief Déclaration de l’interface de l’écran titre d’AkaPong.
 *
 * Ce module expose les fonctions permettant :
 *   - D’afficher l’écran titre avec le logo et le menu principal.
 *   - De récupérer la sélection courante lorsque le joueur valide avec A.
 *
 * Intégration :
 *   - title_screen_show() est appelé dans l’état GameState::Title.
 *   - title_screen_get_selection() est utilisé par app_main.cpp pour
 *     décider du passage à Jouer / Options / Highscores / Retour Loader.
 */

#pragma once

// Affiche l’écran titre + menu principal
void title_screen_show();

// Retourne l’index sélectionné lorsque A est pressé, sinon -1
int title_screen_get_selection();
