/**
 * @file highscores.h
 * @brief Définition des structures et fonctions liées aux meilleurs scores d’AkaPong.
 *
 * Ce module fournit :
 *   - La structure HighscoreEntry (nom + score).
 *   - La constante MAX_SCORES définissant la taille maximale du tableau.
 *   - Les fonctions d’initialisation, de chargement, d’affichage et d’ajout de scores.
 *   - Une interface de saisie interactive du nom du joueur (jusqu’à 8 caractères).
 *
 * Stockage :
 *   - Les scores sont enregistrés dans un fichier binaire situé dans :
 *         /sdcard/AkaPong/AkaPong.sco
 *   - Chaque entrée est un HighscoreEntry (nom + score).
 *   - Le fichier est créé automatiquement si absent.
 *
 * Intégration :
 *   - highscores_submit(score) est appelé automatiquement en fin de partie.
 *   - highscores_show() est utilisé dans l’état GameState::Highscores.
 *   - highscores_input_name() gère la saisie du pseudo avec feedback audio.
 *
 * Ce module est indépendant du moteur du jeu et ne dépend que des couches
 * core/input, core/graphics et core/audio.
 */

#pragma once
#include <vector>
#include <string>
#include "core/persist.h"

// Structure représentant une entrée du tableau des scores.
// name : 8 caractères maximum + '\0'
// score : valeur numérique du score
struct HighscoreEntry {
    char name[9];   // 8 caractères + '\0'
    int score;
};

// Nombre maximum d’entrées affichées et sauvegardées
constexpr int MAX_SCORES = 6;

// Initialise le fichier de scores (création si absent)
void highscores_init();

// Charge toutes les entrées depuis le fichier
std::vector<HighscoreEntry> highscores_load();

// Saisie interactive du nom du joueur (A=valider, C=effacer, B=OK)
std::string highscores_input_name();

// Ajoute un score, trie le tableau et sauvegarde
void highscores_submit(int score);

// Affiche le tableau des scores
void highscores_show();
