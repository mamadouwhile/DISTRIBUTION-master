#include "joueur_random.h"

Joueur_Random::Joueur_Random(std::string nom, bool joueur)
    :Joueur(nom,joueur)
{}


void Joueur_Random::initialisation()
{
}

void Joueur_Random::init_partie()
{
}


char Joueur_Random::nom_abbrege() const
{
    return 'R';
}

void Joueur_Random::recherche_coup(Jeu jeu, int &coup)
{
  if (jeu.nb_coups() == 1) {
    coup = 1;
  } else {
    coup = (rand() % jeu.nb_coups()) + 1;
  }
}
