#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#define RACINE 0

class Etat {
 public :
  int _val;
};

class Jeu {
 private :
  Etat _etat;

 public :
  Jeu();
  void reset(); 
  bool coup_licite(int); // Vérifie si le coup est licite
  void joue(int); // Joue le coup (sans vérifier qu'il est licite)
  int nb_coups(); // Le nombre de coups possibles
  bool terminal(); // Vérifie si un état final a été atteint (dans ce cas le jeu prend fin)
  bool pat();
  bool victoire();
};
