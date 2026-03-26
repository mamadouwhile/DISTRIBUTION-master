#pragma once

#include "joueur.h"
#include <vector>
#include <random>

class Joueur_MCTS : public Joueur
{
private:
    unsigned int _etat;
    std::vector<int> _historique_partie;
    std::mt19937 _rng;

public:
    Joueur_MCTS(std::string nom, bool joueur);

    void initialisation() override;
    void init_partie() override;
    char nom_abbrege() const override;
    void recherche_coup(Jeu jeu, int & coup) override;
};