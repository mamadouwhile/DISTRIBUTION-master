#pragma once
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <utility>
#include <functional>
#include <vector>
#include "jeu.h"
#include "joueurs/joueur.h"
#include "joueurs/joueur_random.h"
#include "joueurs/joueur_brutal.h"
#include "joueurs/joueur_MCTS.h"

#define TEMPS_POUR_UN_COUP 10 // millisecondes
#define COUP_INVALIDE -1000
#define JOUEUR_1 1
#define JOUEUR_2 2
#define VICTOIRE 1
#define DEFAITE -1
#define PAT 0

/**********************************************************************************
 *             classe lançant la partie et qui se sert des                        *
 *             mutex pour s'assurer que les joueurs joue                          *
 *             dans les temps                                                     *
 **********************************************************************************/

enum class player {MCTS , BRUTAL , RAND } ;

/**
 * @brief The Arbitre class
 * classe gerant la partie et les tours des joueurs.
 * si un joueur ne joue pas dans les temps, il perd,
 * si un joueur ne joue pas un coup correct (case sur le pateau et vide), il perd.
 */
class Arbitre
{
private:
    std::shared_ptr<Joueur> _joueur1;
    std::shared_ptr<Joueur> _joueur2;

    Jeu _jeu;
    std::vector<int> _coups;

    std::vector<std::mutex> _coups_mutex;

    int _nombre_parties;
    int _numero_partie;

    player _player1;
    player _player2;

public:
    /**
     * @brief Arbitre
     * @param player1
     * @param player2
     * @param nombre_parties
     */
    Arbitre(player player1 , player player2, int nombre_parties);


    void initialisation();


    /**
     * @brief challenge
     * @param nombre_parties
     * methode lançant un challenge avec un nombre de parties
     * affiche le nombre de parties gagnées pour chaque participant
     */
    void challenge();

    /**
     * @brief partie
     * @return le gagnant de la partie (joueur 1 : 1, joueur 2 : 0)
     * lance une partie,
     *
     */
    int partie();
};
