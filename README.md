# Joueurs MCTS pour un moteur de jeu fourni en `jeu.o`

## 1. Presentation du projet

Ce projet contient une implementation de Monte Carlo Tree Search (MCTS) pour un moteur de jeu fourni sous forme binaire, via `jeu.o`.

L'objectif n'est pas de coder une strategie parfaite pour un jeu connu d'avance, mais de proposer un joueur capable de fonctionner dans un cadre partiellement opaque, avec un moteur deja compile et peu transparent.

Le depot contient maintenant deux versions de travail du joueur MCTS :

- `joueurs/joueur_MCTS.cpp` et `joueurs/joueur_MCTS.h` : la version principale actuellement compilee par le projet
- `joueurs/joueur_MCTS_basique/joueur_MCTS.cpp` et `joueurs/joueur_MCTS_basique/joueur_MCTS.h` : une version basique rangee dans un sous-dossier de remplacement

Le `CMakeLists.txt` des joueurs ne compile que la version principale. La version basique est volontairement stockee a part, sous les memes noms de fichiers et de classe, pour qu'un correcteur ou un enseignant puisse la tester facilement en remplacant simplement les deux fichiers `joueur_MCTS.*` du dossier `joueurs`.

## 2. Idee generale de Monte Carlo Tree Search

Monte Carlo Tree Search repose sur quatre etapes principales.

### Selection
On descend dans l'arbre a partir de la racine en choisissant a chaque noeud l'enfant qui semble le plus prometteur.

Le choix se fait avec une formule de type UCB, qui combine :

- l'exploitation : favoriser les coups qui ont deja donne de bons resultats
- l'exploration : continuer a essayer des branches encore peu visitees

Le but est de ne pas se bloquer trop vite sur une seule branche, tout en concentrant progressivement le temps de calcul sur les lignes interessantes.

### Expansion
Quand on atteint un noeud non terminal qui possede encore des coups non explores, on en ouvre un nouveau dans l'arbre.

### Simulation / Rollout
A partir de cet etat, on joue une suite de coups simules jusqu'a une certaine profondeur ou jusqu'a une fin de partie.

### Retropropagation
Le resultat de la simulation remonte ensuite dans tous les noeuds traverses. Chaque noeud met a jour :

- son nombre de visites
- son score cumule

Au fil des iterations, l'arbre accumule donc une estimation statistique de la qualite des coups.

### Role de UCB
Le critere UCB sert a equilibrer exploration et exploitation.

Concretement :

- si l'exploration est trop faible, le joueur se fixe trop vite sur quelques branches
- si elle est trop forte, le joueur disperse son temps de calcul et n'apprend pas assez sur les meilleures branches

Dans ce projet, cet equilibre est principalement regle par `C_EXPLORATION`.

## 3. Organisation des deux versions

## Version principale : `joueurs/joueur_MCTS.*`

Cette version reste un MCTS, mais elle ajoute quelques decisions pragmatiques avant la recherche statistique.

### Ameliorations
Avant de lancer l'arbre, le joueur :

- cherche un coup gagnant immediat
- essaie d'eliminer les coups qui donnent une victoire immediate a l'adversaire

Le rollout n'est pas purement uniforme. Les coups sont tries selon une regle simple, puis le joueur tire parmi un petit sous-ensemble. L'idee est de reduire un peu le bruit des simulations sans ecrire une heuristique specialisee du jeu.

### Objectif
Cette version vise a etre plus solide qu'un MCTS minimal, tout en restant compatible avec les contraintes du moteur fourni.

## Version basique de remplacement : `joueurs/joueur_MCTS_basique/joueur_MCTS.*`

Cette version cherche a rester un MCTS plus simple :

- pas de filtre tactique avant la recherche
- pas de detection explicite d'un gain immediat avant l'arbre
- rollout purement aleatoire

Elle est stockee dans un sous-dossier et non compilee par defaut. L'interet de cette organisation est pratique : si l'on souhaite tester la version basique a la place de la version principale, il suffit de :

1. supprimer ou mettre de cote `joueurs/joueur_MCTS.cpp` et `joueurs/joueur_MCTS.h`
2. deplacer `joueurs/joueur_MCTS_basique/joueur_MCTS.cpp` et `joueurs/joueur_MCTS_basique/joueur_MCTS.h` directement dans `joueurs/`
3. recompiler

Ainsi, aucun renommage manuel n'est necessaire dans le code source.

## 4. Particularite importante du moteur `jeu.o`

C'est le point technique central du projet.

En pratique, le moteur fourni sous forme de `jeu.o` semble utiliser un etat global partage, ou au minimum un comportement qui rend les objets `Jeu` peu fiables comme copies totalement independantes.

Autrement dit, il n'est pas prudent de supposer que :

```cpp
Jeu copie = jeu;
```

donne toujours un etat autonome, isole, manipulable librement dans une simulation sans effet de bord.

### Consequence directe
Les joueurs utilisent donc un historique interne des coups : `_historique_partie`.

Avant certaines operations importantes, ils reconstruisent un etat coherent par la sequence suivante :

1. `reset()`
2. rejeu des coups de `_historique_partie`

### Pourquoi cette reconstruction
Cette reconstruction sert a repartir d'un etat aussi coherent que possible avant :

- les tests tactiques
- les simulations
- la verification finale du coup choisi

### Pourquoi l'etat est restaure avant de rendre le coup
Comme la recherche MCTS manipule plusieurs etats temporaires, le code restaure explicitement l'etat courant avant de verifier puis de renvoyer le coup final. C'est un choix pragmatique, motive par les limites du moteur fourni.

## 5. Parametres importants et comment les regler

## `MAX_TIME_MS`

### Ce que ce parametre controle
`MAX_TIME_MS` fixe le budget temps interne du joueur pour choisir un coup.

### Si on augmente `MAX_TIME_MS`
- le joueur peut faire plus d'iterations
- l'estimation statistique peut devenir meilleure
- mais on se rapproche de la limite reelle imposee par l'arbitre

### Si on le diminue
- le joueur joue plus vite
- il y a moins de risque de depassement de temps
- mais l'arbre accumule moins d'information

### Point important avec l'arbitre
Si l'arbitre affiche `mutex non rendu`, il faut reduire `MAX_TIME_MS`.

Meme si des tests locaux ne montrent pas d'erreur a `9 ms`, il peut rester un risque a cause :

- des approximations de mesure
- de la machine utilisee
- de l'ordonnancement des threads
- du comportement exact de l'arbitre

Garder une petite marge de securite est prudent.

## `C_EXPLORATION`

### Ce que ce parametre controle
`C_EXPLORATION` regle le poids du terme d'exploration dans UCB.

### Si on l'augmente
- le joueur essaie plus de branches peu visitees
- il explore plus large
- mais il peut perdre du temps sur des branches peu prometteuses

### Si on le diminue
- le joueur se concentre plus vite sur les coups deja juges bons
- il exploite davantage
- mais il risque de passer a cote d'une branche meilleure encore peu testee

## `MAX_ROLLOUT_DEPTH`

### Ce que ce parametre controle
`MAX_ROLLOUT_DEPTH` limite la longueur des simulations apres l'expansion.

### Si on l'augmente
- chaque rollout va plus loin
- certains resultats peuvent etre plus informatifs
- mais chaque iteration coute plus cher
- on fait donc moins d'iterations au total

### Si on le diminue
- on peut faire plus de simulations
- mais les rollouts deviennent plus courts et parfois moins utiles

## 6. Limites et pistes d'amelioration

### Cout de reconstruction d'etat
Rejouer l'historique avant les simulations a un cout important. Sur un budget temps court, ce cout devient vite sensible.

### Dependance au moteur fourni
Le comportement de `jeu.o` conditionne fortement les choix d'implementation. Certaines techniques plus propres en theorie deviennent ici fragiles ou peu fiables.

### Sensibilite aux parametres
Les performances changent vite quand on modifie :

- le temps disponible
- l'exploration
- la profondeur des rollouts

Ces parametres doivent donc etre ajustes empiriquement.

### Amelioration des rollouts
La version principale guide deja un peu les rollouts, mais on pourrait encore tester :

- des politiques de rollout moins bruitees
- des coupures mieux choisies
- des heuristiques locales plus informatives

### Politique de selection finale
Le choix final pourrait encore etre affiné selon les tests :

- meilleur score moyen
- meilleur nombre de visites
- combinaison des deux selon le type d'adversaire

### Optimisation memoire
L'arbre alloue beaucoup de petits objets et reconstruit souvent des vecteurs de coups. Des optimisations memoire peuvent faire gagner du temps reel.

## 7. Conclusion

Le projet propose maintenant une structure simple a manipuler :

- une version `joueur_MCTS` active, compilee par defaut
- une version `joueur_MCTS` basique de remplacement, stockee dans un sous-dossier pret a etre copie dans `joueurs/`

La version principale reste un MCTS avec quelques heuristiques pragmatiques. La version de remplacement reste un MCTS plus pur et plus minimal. Cette organisation permet de comparer facilement les deux approches sans demander de renommage manuel dans les fichiers au moment du test.

Projet realise par Walid SMIHI et Mahamadou Dembele dans le cadre du cours d'initiation aux systemes intelligents dispense par M. Igor Stephan.
