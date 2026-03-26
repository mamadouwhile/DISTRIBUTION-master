#pragma once

#include <mutex>
#include <iostream>
#include "../jeu.h"

/*****************************************************************************************************
 *      Toutes les classes implantant les divers joueurs doivent hériter de cette classe             *
 *      et donc contenir ces methodes                                                                *
 *****************************************************************************************************/

/**
 * @brief The Joueur class
 * classe mère des joueurs
 * chaque joueur doit implémenter la méthode jouer
 */
class Joueur
{
  std::string _nom;   //nom du joueur
  bool _joueur;       //joueur 1 : true , joueur 2 : false

public:
  /**
   * @brief Joueur
   * @param nom
   * @param joueur
   * création du joueur, joueur correspond au numero du joueur ( joueur 1 : true, joueur 2 : false )
   */
  Joueur(std::string nom, bool joueur);
  
  /**
   * @brief jouer
   * @param jeu 
   * @param coup : le coup doit etre modifié par le joueur avant la fin du temps imparti
   * @param coup_mutex : le mutex doit etre pris par le joueur au début de la partie puis rendu avant le temps imparti
   */
  virtual void jouer(Jeu jeu, int & coup , std::mutex & coup_mutex) final;
  
  /**
   * @brief recherche_coup
   * @param jeu
   * @param coup : le coup doit etre modifié par le joueur avant la fin du temps imparti
   */
  virtual void recherche_coup(Jeu, int & coup) =0;


  /**
   * @brief initialisation
   */
  virtual void initialisation() =0;
    
  /**
   * @brief init_partie
   */
  virtual void init_partie() =0;


  /**
   * @brief nom
   * @return le nom du joueur
   */
  std::string nom() const;
  
  /**
   * @brief joueur
   * @return le numero du joueur ( joueur 1 : true, joueur 2 : false )
   */
  bool joueur() const;
  
  /**
     * @brief setJoueur
     * @param joueur ( joueur 1 : true, joueur 2 : false )
     */
    void setJoueur(bool joueur);

  virtual char nom_abbrege() const =0;

};
