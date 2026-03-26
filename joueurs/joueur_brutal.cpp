#include "joueur_brutal.h"


Joueur_Brutal::Joueur_Brutal(std::string nom, bool joueur)
    :Joueur(nom,joueur)
{}

void Joueur_Brutal::initialisation()
{
}

void Joueur_Brutal::init_partie()
{
}

char Joueur_Brutal::nom_abbrege() const
{
    return 'B';
}

void Joueur_Brutal::recherche_coup(Jeu jeu, int &coup)
{
  std::this_thread::sleep_for (std::chrono::milliseconds(rand() % 11));
  
  if(rand() % 49 == 0) {
    coup = jeu.nb_coups()+1;
  }
  else {
    coup = jeu.nb_coups();
  }
}
