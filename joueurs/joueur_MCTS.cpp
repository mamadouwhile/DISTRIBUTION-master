#include "joueur_MCTS.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <limits>

namespace {
    // Budget temps interne du joueur.
    // Il doit rester en dessous de la limite imposee par l'arbitre.
    const int MAX_TIME_MS = 9;

    // Coefficient UCB : plus il est grand, plus on explore des branches peu visitees.
    // Plus il est petit, plus on exploite vite les branches deja prometteuses.
    const double C_EXPLORATION = 0.5;

    // Profondeur maximale des simulations aleatoires.
    // Une profondeur courte permet plus d'iterations, une profondeur grande donne
    // des simulations plus longues mais plus couteuses.
    const int MAX_ROLLOUT_DEPTH = 16;

    // Enumerer les coups legaux a partir de l'etat courant du moteur.
    std::vector<int> coups_legaux(Jeu& jeu) {
        std::vector<int> coups;
        int nb = jeu.nb_coups();
        for (int i = 1; i <= nb; ++i) {
            if (jeu.coup_licite(i)) {
                coups.push_back(i);
            }
        }
        return coups;
    }

    // Noeud classique d'arbre MCTS.
    // - coup_origine : coup joue depuis le parent pour arriver ici
    // - score : somme des resultats retropropages
    // - visites : nombre de passages dans ce noeud
    // - tour_racine : indique si le noeud correspond au camp du joueur racine
    // - coups_possibles : coups encore non explores depuis ce noeud
    struct NoeudMCTS {
        int coup_origine;
        double score = 0.0;      // score du point de vue de la racine
        int visites = 0;
        bool tour_racine;
        NoeudMCTS* parent;
        std::vector<NoeudMCTS*> enfants;
        std::vector<int> coups_possibles;

        NoeudMCTS(int c, NoeudMCTS* p, const std::vector<int>& legaux, bool tr)
            : coup_origine(c), tour_racine(tr), parent(p), coups_possibles(legaux) {}

        ~NoeudMCTS() {
            for (auto e : enfants) delete e;
        }
    };
}

Joueur_MCTS::Joueur_MCTS(std::string nom, bool joueur)
    : Joueur(nom, joueur), _etat(42), _rng(42) {}

void Joueur_MCTS::initialisation() {
    // On change la graine a chaque initialisation pour eviter de rejouer
    // toujours les memes trajectoires pseudo-aleatoires.
    _rng.seed(_etat++);
}

void Joueur_MCTS::init_partie() {
    // L'historique n'a de sens qu'a l'echelle d'une seule partie.
    _historique_partie.clear();
}

char Joueur_MCTS::nom_abbrege() const {
    return 'M';
}

void Joueur_MCTS::recherche_coup(Jeu jeu, int& coup) {
    auto start = std::chrono::steady_clock::now();

    auto rand_index = [&](int n) -> int {
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(_rng);
    };

    auto timeout = [&]() -> bool {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= MAX_TIME_MS;
    };

    // Point important du projet : le moteur fourni sous forme de jeu.o semble
    // partager un etat global entre objets Jeu. On ne peut donc pas compter sur
    // des copies independantes de Jeu pour simuler librement.
    // Le compromis retenu ici consiste a reconstruire un etat coherent depuis
    // l'historique des coups, via reset() puis rejeu des coups memorises.
    auto reconstruire = [&](Jeu& handle) -> bool {
        handle.reset();
        for (int m : _historique_partie) {
            if (!handle.coup_licite(m)) {
                return false;
            }
            handle.joue(m);
        }
        return true;
    };

    // Evalue rapidement l'effet d'un coup sans lancer une recherche complete.
    // Valeurs retournees :
    // - 1 : victoire immediate pour le joueur MCTS
    // -  0 : partie nulle immediate
    // - -1 : defaite immediate
    // -  2 : etat non terminal
    // - -3 : etat incoherent / coup impossible pendant la reconstruction
    auto resultat_apres_coup = [&](int c) -> int {
        Jeu t;
        if (!reconstruire(t)) return -3;
        if (!t.coup_licite(c)) return -3;
        t.joue(c);
        if (!t.terminal()) return 2;
        if (t.pat()) return 0;
        return (t.victoire() == joueur()) ? 1 : -1;
    };

    // Filtre tactique 1 : si un coup gagne immediatement, pass besoin de lancer
    // le MCTS complet sur cette position.
    auto coup_gagnant_immediat = [&](const std::vector<int>& legaux, int& move) -> bool {
        for (int c : legaux) {
            if (resultat_apres_coup(c) == 1) {
                move = c;
                return true;
            }
            if (timeout()) {
                return false;
            }
        }
        return false;
    };

    // Filtre tactique 2 : on essaie d'eliminer les coups qui donnent une victoire
    // immediate a l'adversaire. Ce n'est pas une recherche profonde, juste un
    // garde-fou local avant de depenser le budget MCTS.
    auto coups_non_perdants = [&](const std::vector<int>& valides) -> std::vector<int> {
        std::vector<int> bons;
        for (int c : valides) {
            Jeu apres;
            if (!reconstruire(apres)) continue;
            if (!apres.coup_licite(c)) continue;
            apres.joue(c);

            if (apres.terminal()) {
                bons.push_back(c);
                continue;
            }

            bool perd_immediatement = false;
            auto reps = coups_legaux(apres);
            for (int r : reps) {
                Jeu t;
                if (!reconstruire(t)) {
                    perd_immediatement = true;
                    break;
                }
                if (!t.coup_licite(c)) {
                    perd_immediatement = true;
                    break;
                }
                t.joue(c);
                if (!t.coup_licite(r)) continue;
                t.joue(r);

                if (t.terminal() && !t.pat() && t.victoire() != joueur()) {
                    perd_immediatement = true;
                    break;
                }
                if (timeout()) {
                    perd_immediatement = true;
                    break;
                }
            }

            if (!perd_immediatement) {
                bons.push_back(c);
            }

            if (timeout()) break;
        }
        return bons;
    };

    // On remet d'abord le moteur dans un etat coherent correspondant a
    // l'historique memorise de la partie courante.
    if (!reconstruire(jeu)) {
        _historique_partie.clear();
        jeu.reset();
    }

    // Dans ce projet, le parametre coup sert aussi a recuperer le dernier coup
    // connu avant que le joueur choisisse le sien. Si ce coup est coherent dans
    // l'etat reconstruit, on l'ajoute a l'historique.
    if (coup > 0) {
        if (jeu.coup_licite(coup)) {
            _historique_partie.push_back(coup);
            jeu.joue(coup);
        } else {
            _historique_partie.clear();
            jeu.reset();
        }
    }

    std::vector<int> valides = coups_legaux(jeu);
    int nb_c = jeu.nb_coups();

    if (nb_c <= 0 || valides.empty()) {
        coup = 1;
        return;
    }

    if ((int)valides.size() == 1) {
        coup = valides[0];
        if (!reconstruire(jeu)) {
            jeu.reset();
        }
        _historique_partie.push_back(coup);
        return;
    }

    // Priorite absolue : si un coup gagne maintenant, on le joue sans ouvrir l'arbre.
    {
        int win_now = -1;
        if (coup_gagnant_immediat(valides, win_now)) {
            if (!reconstruire(jeu)) {
                jeu.reset();
            }
            coup = win_now;
            _historique_partie.push_back(coup);
            return;
        }
    }

    // Si possible, on restreint la racine aux coups qui ne perdent pas tout de suite.
    {
        std::vector<int> surs = coups_non_perdants(valides);
        if (!surs.empty()) {
            valides = surs;
        }
    }

    // Coup de secours si la recherche n'a pas le temps d'installer des statistiques utiles.
    int meilleur_coup = valides[rand_index((int)valides.size())];
    NoeudMCTS* racine = new NoeudMCTS(-1, nullptr, valides, true);

    while (!timeout()) {
        Jeu simu;
        if (!reconstruire(simu)) {
            break;
        }

        NoeudMCTS* courant = racine;

        // SELECTION : on descend dans l'arbre avec UCB.
        // Aux noeuds de l'adversaire, on inverse le terme d'exploitation pour
        // reflecter le fait qu'un bon coup pour lui est mauvais pour la racine.
        while (!simu.terminal() && courant->coups_possibles.empty() && !courant->enfants.empty()) {
            NoeudMCTS* meilleur = nullptr;
            double meilleur_ucb = -std::numeric_limits<double>::infinity();
            double log_parent = std::log((double)std::max(1, courant->visites));

            for (auto e : courant->enfants) {
                double ucb;
                if (e->visites == 0) {
                    ucb = 1e9;
                } else {
                    double wr = e->score / e->visites;
                    double exploitation = courant->tour_racine ? wr : (1.0 - wr);
                    double exploration = C_EXPLORATION * std::sqrt(log_parent / e->visites);
                    ucb = exploitation + exploration;
                }

                if (ucb > meilleur_ucb) {
                    meilleur_ucb = ucb;
                    meilleur = e;
                }
            }

            if (!meilleur) break;
            courant = meilleur;

            if (!simu.coup_licite(courant->coup_origine)) {
                break;
            }
            simu.joue(courant->coup_origine);
        }

        // EXPANSION : on ouvre un nouvel enfant si le noeud n'est pas terminal
        // et qu'il reste des coups encore non explores.
        if (!simu.terminal() && !courant->coups_possibles.empty()) {
            int idx = rand_index((int)courant->coups_possibles.size());
            int c = courant->coups_possibles[idx];

            courant->coups_possibles[idx] = courant->coups_possibles.back();
            courant->coups_possibles.pop_back();

            if (simu.coup_licite(c)) {
                simu.joue(c);
                std::vector<int> nv = coups_legaux(simu);
                NoeudMCTS* enfant = new NoeudMCTS(c, courant, nv, !courant->tour_racine);
                courant->enfants.push_back(enfant);
                courant = enfant;
            }
        }

        // SIMULATION / ROLLOUT : cette version n'est pas totalement uniforme.
        // Les coups sont tries pour privilegier ceux proches du "milieu", puis
        // on tire parmi les meilleurs candidats de cette liste. L'idee est de
        // reduire un peu le bruit du rollout sans introduire une heuristique trop forte.
        // (obtenu grace à des tests sur le jeu mystere,
        // on comprend vite que c'est un puissance 4)
        int prof = 0;
        while (!simu.terminal() && prof < MAX_ROLLOUT_DEPTH) {
            auto legaux = coups_legaux(simu);
            if (legaux.empty()) break;

            int milieu = ((int)legaux.size() + 1) / 2;
            std::sort(legaux.begin(), legaux.end(), [&](int a, int b) {
                int da = std::abs(a - milieu);
                int db = std::abs(b - milieu);
                if (da != db) return da < db;
                return a < b;
            });

            int k = std::min<int>(3, (int)legaux.size());
            int next_move = legaux[rand_index(k)];

            if (!simu.coup_licite(next_move)) break;
            simu.joue(next_move);
            prof++;
        }

        // RETROPROPAGATION : le score terminal remonte jusqu'a la racine.
        // Cette version reste simple : 1.0 victoire, 0.5 pat, 0.0 defaite.
        double score_final = 0.5;
        if (simu.terminal()) {
            if (simu.pat()) {
                score_final = 0.5;
            } else if (simu.victoire() == joueur()) {
                score_final = 1.0;
            } else {
                score_final = 0.0;
            }
        }

        while (courant) {
            courant->visites++;
            courant->score += score_final;
            courant = courant->parent;
        }
    }

    // Avant de rendre le coup, on restaure l'etat courant pour verifier la validite
    // du coup retenu dans un etat aussi coherent que possible.
    if (!reconstruire(jeu)) {
        _historique_partie.clear();
        jeu.reset();
        valides = coups_legaux(jeu);
        nb_c = jeu.nb_coups();
    } else {
        valides = coups_legaux(jeu);
        nb_c = jeu.nb_coups();
    }

    // Choix final : on privilegie la meilleure moyenne, puis on departage avec
    // le nombre de visites si plusieurs coups ont des scores tres proches.
    double meilleur_wr = -1.0;
    int meilleures_visites = -1;
    for (auto e : racine->enfants) {
        if (e->visites == 0) continue;

        double wr = e->score / e->visites;
        if (wr > meilleur_wr + 1e-12 ||
            (std::abs(wr - meilleur_wr) <= 1e-12 && e->visites > meilleures_visites)) {
            meilleur_wr = wr;
            meilleures_visites = e->visites;
            meilleur_coup = e->coup_origine;
        }
    }

    // Garde-fou final : si le coup choisi n'est plus legal dans l'etat courant,
    // on retombe sur un coup legal disponible a la racine.
    if (valides.empty()) {
        meilleur_coup = 1;
    } else if (meilleur_coup < 1 || meilleur_coup > nb_c || !jeu.coup_licite(meilleur_coup)) {
        meilleur_coup = valides[rand_index((int)valides.size())];
    }

    delete racine;

    coup = meilleur_coup;
    _historique_partie.push_back(coup);
}
