#pragma once


#include <thread>
#include "joueur.h"


class Joueur_Brutal : public Joueur
{
private:

public:
  Joueur_Brutal(std::string nom, bool joueur);

  void initialisation() override;

  void init_partie() override;

  char nom_abbrege() const override;
  
  void recherche_coup(Jeu, int & coup) override;

};
