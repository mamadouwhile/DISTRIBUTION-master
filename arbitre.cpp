#include "arbitre.h"


Arbitre::Arbitre(player player1, player player2, int nombre_parties)
  :
   _jeu(),
   _coups(nombre_parties, COUP_INVALIDE),
   _coups_mutex(nombre_parties),
   _nombre_parties(nombre_parties),
   _numero_partie(1),
   _player1(player1),
   _player2(player2)
{}

void Arbitre::initialisation()
{
  _joueur1=nullptr;
  _joueur2=nullptr;
  
  _jeu.reset();
  
  switch ((_numero_partie%2? _player1 : _player2)) {
  case player::MCTS:
    _joueur1 = std::make_shared<Joueur_MCTS> ("MCTS",true);
    break;
  case player::BRUTAL:
    _joueur1 = std::make_shared<Joueur_Brutal> ("Brutal",true);
    break;
  case player::RAND:
    _joueur1 = std::make_shared<Joueur_Random> ("Random",true);
    break;
  default:
    break;
  }
  
  switch ((!(_numero_partie%2)? _player1 : _player2)) {
  case player::MCTS:
    _joueur2 = std::make_shared<Joueur_MCTS> ("MCTS",false);
    break;
  case player::BRUTAL:
    _joueur2 = std::make_shared<Joueur_Brutal> ("Brutal",false);
    break;
  case player::RAND:
    _joueur2 = std::make_shared<Joueur_Random> ("Random",false);
    break;
  default:
    break;
  }
  
}

void Arbitre::challenge()
{
  initialisation(); // Au moins une fois pour que les objets de la ligne qui suit soient définis
  
  _joueur1->initialisation();
  _joueur2->initialisation();
    std::cout << "Le challenge de " << _nombre_parties << " parties "
              <<"entre " << _joueur1->nom() << " et " << _joueur2->nom()
             << " commence. " << std::endl;
    int victoire_joueur_1 = 0;
    int victoire_joueur_2 = 0;
    for(int i=0 ; i < _nombre_parties ; i++)
    {
        std::cout << "\n" << "Partie n°" << _numero_partie << " : ";
	int resultat = partie();
	if (resultat != PAT) 
	  (resultat == VICTOIRE ?
	   ((_numero_partie%2)?
	    victoire_joueur_1++
	    :
	    victoire_joueur_2++ )
	   :
	   (!(_numero_partie%2)?
	    victoire_joueur_1++
	    :
	    victoire_joueur_2++ ));
        std::this_thread::sleep_for (std::chrono::milliseconds(250)); // temps de latence entre deux parties
        _numero_partie++;
        initialisation();
    }
    std::cout << "FIN DU CHALLENGE\n\t"
              << _joueur1->nom()<< " gagne " << ((_numero_partie%2)? victoire_joueur_1 : victoire_joueur_2)
              << "\n\t"<< _joueur2->nom()<< " gagne " << ((_numero_partie%2) ? victoire_joueur_2 : victoire_joueur_1) << std::endl;
}

int Arbitre::partie()
{
  int tour = 0;
  _joueur1->init_partie();
  _joueur2->init_partie();
  while(!_jeu.terminal())
    {
      bool try_lock = false;
      bool coup_invalide = false;
      bool coup_illicite = false;
      tour++;

      _coups_mutex[_numero_partie-1].unlock();
      
      std::thread thread_joueur(&Joueur::jouer,
				((tour%2)? (_joueur1) :(_joueur2) ),
				_jeu,
				std::ref(_coups[_numero_partie-1]),
				std::ref(_coups_mutex[_numero_partie-1]));
      
      std::this_thread::sleep_for (std::chrono::milliseconds(TEMPS_POUR_UN_COUP));
      
      if (!_coups_mutex[_numero_partie-1].try_lock()) {
	std::cerr <<  std::endl << "mutex non rendu " << std::endl;
	try_lock = true;
      }
      else if(_coups[_numero_partie-1] == COUP_INVALIDE) {
	std::cerr << "coup invalide -1" << std::endl;
	coup_invalide = true;
      }
      else if(!_jeu.coup_licite(_coups[_numero_partie-1])) {
	std::cerr << "coup illicite " << _coups[_numero_partie-1] << std::endl;
	coup_illicite = true;
      }

      thread_joueur.detach();
 
      if(try_lock || coup_invalide || coup_illicite)
	{
	  if(tour%2)
	    {
	      std::cout << _joueur2->nom() <<" (2) gagne ! Nombre de tours : " << tour << std::endl;  
	      return JOUEUR_2; // joueur 2 gagne
	    }
	  else
	    {
	      std::cout << _joueur1->nom() <<" (1) gagne ! Nombre de tours : " << tour << std::endl;
	      return JOUEUR_1; // joueur 1 gagne
	    }
	}

      _jeu.joue(_coups[_numero_partie-1]);
      
      std::cout << ((tour%2) ? _joueur1->nom_abbrege() : _joueur2->nom_abbrege()) << _coups[_numero_partie-1] << std::endl;

    } // fin while
  if (_jeu.pat())
    {
      std::cout << std::endl << "Partie nulle" << std::endl;
      return 0;
    }
  else
    {
      int resultat = (_jeu.victoire()) ? VICTOIRE : DEFAITE;
      std::cout << std::endl << ((resultat == VICTOIRE)? "Victoire" : "Défaite")  << " en " << tour-1 << " tours." << std::endl;
      return resultat;
    }
}
