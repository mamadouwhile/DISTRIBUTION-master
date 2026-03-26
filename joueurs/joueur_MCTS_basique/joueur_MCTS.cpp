#include "joueur_MCTS.h"
#include <vector>
#include <cmath>
#include <chrono>
#include <random>
#include <limits>

namespace {
    // Budget temps interne du joueur.
    // Cette version respecte les memes contraintes que la version MCTS amelioree.
    const int MAX_TIME_MS = 9;

    // Coefficient UCB : regle l'equilibre exploration / exploitation.
    const double C_EXPLORATION = 0.5;

    // Profondeur maximale du rollout aleatoire.
    const int MAX_ROLLOUT_DEPTH = 16;

    // Enumerer les coups legaux dans l'etat courant.
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

    // Structure de noeud standard pour le MCTS.
    // Cette version basique garde la meme structure generale que la version
    // amelioree, mais sans filtres tactiques avant la recherche.
    struct NoeudMCTS {
        int coup_origine;
        double score = 0.0;   // score du point de vue de la racine
        int visites = 0;
        bool tour_racine;
        NoeudMCTS* parent;
        std::vector<NoeudMCTS*> enfants;
        std::vector<int> coups_possibles;

        NoeudMCTS(int c, NoeudMCTS* p, const std::vector<int>& legaux, bool tr)
            : coup_origine(c), tour_racine(tr), parent(p), coups_possibles(legaux) {}

        ~NoeudMCTS() {
            for (auto e : enfants) {
                delete e;
            }
        }
    };
}

Joueur_MCTS::Joueur_MCTS(std::string nom, bool joueur)
    : Joueur(nom, joueur), _etat(42), _rng(42) {}

void Joueur_MCTS::initialisation() {
    _rng.seed(_etat++);
}

void Joueur_MCTS::init_partie() {
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

    // Comme pour l'autre joueur, on reconstruit l'etat depuis l'historique.
    // C'est un compromis impose par le moteur jeu.o, qui ne se comporte pas
    // comme un ensemble d'objets Jeu parfaitement independants.
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

    // Restaure l'etat courant depuis l'historique connu.
    if (!reconstruire(jeu)) {
        _historique_partie.clear();
        jeu.reset();
    }

    // Le dernier coup recu est reintegre a l'historique avant la recherche.
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
        _historique_partie.push_back(coup);
        return;
    }

    // Si le budget temps est trop court pour construire des statistiques utiles,
    // on garde au moins un coup legal de repli.
    int meilleur_coup = valides[rand_index((int)valides.size())];
    NoeudMCTS* racine = new NoeudMCTS(-1, nullptr, valides, true);

    while (!timeout()) {
        Jeu simu;
        if (!reconstruire(simu)) {
            break;
        }

        NoeudMCTS* courant = racine;

        // SELECTION : descente dans l'arbre avec UCB, sans filtre tactique prealable.
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

            if (meilleur == nullptr) {
                break;
            }

            courant = meilleur;

            if (!simu.coup_licite(courant->coup_origine)) {
                break;
            }
            simu.joue(courant->coup_origine);
        }

        // EXPANSION : on ouvre un nouveau fils sur un coup encore non explore.
        if (!simu.terminal() && !courant->coups_possibles.empty()) {
            int idx = rand_index((int)courant->coups_possibles.size());
            int c = courant->coups_possibles[idx];

            courant->coups_possibles[idx] = courant->coups_possibles.back();
            courant->coups_possibles.pop_back();

            if (simu.coup_licite(c)) {
                simu.joue(c);
                std::vector<int> nouveaux_valides = coups_legaux(simu);
                NoeudMCTS* enfant = new NoeudMCTS(c, courant, nouveaux_valides, !courant->tour_racine);
                courant->enfants.push_back(enfant);
                courant = enfant;
            }
        }

        // SIMULATION / ROLLOUT : dans cette version, le rollout reste volontairement
        // purement aleatoire. C'est la principale difference avec la version MCTS
        // amelioree, qui ajoute un peu de guidage tactique.
        int prof = 0;
        while (!simu.terminal() && prof < MAX_ROLLOUT_DEPTH) {
            std::vector<int> legaux = coups_legaux(simu);
            if (legaux.empty()) {
                break;
            }

            int next_move = legaux[rand_index((int)legaux.size())];
            if (!simu.coup_licite(next_move)) {
                break;
            }

            simu.joue(next_move);
            prof++;
        }

        // RETROPROPAGATION : on remonte le resultat du rollout vers la racine.
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

    // Restauration finale de l'etat avant verification du coup choisi.
    if (!reconstruire(jeu)) {
        _historique_partie.clear();
        jeu.reset();
        valides = coups_legaux(jeu);
        nb_c = jeu.nb_coups();
    } else {
        valides = coups_legaux(jeu);
        nb_c = jeu.nb_coups();
    }

    // Choix final par meilleure moyenne, avec tie-break sur les visites.
    double meilleur_wr = -1.0;
    int meilleures_visites = -1;
    for (auto e : racine->enfants) {
        if (e->visites == 0) {
            continue;
        }

        double wr = e->score / e->visites;
        if (wr > meilleur_wr + 1e-12 ||
            (std::abs(wr - meilleur_wr) <= 1e-12 && e->visites > meilleures_visites)) {
            meilleur_wr = wr;
            meilleures_visites = e->visites;
            meilleur_coup = e->coup_origine;
        }
    }

    // Si le meilleur coup n'est plus legal dans l'etat courant, on reprend un
    // coup legal disponible plutot que de renvoyer un coup incoherent.
    if (valides.empty()) {
        meilleur_coup = 1;
    } else if (meilleur_coup < 1 || meilleur_coup > nb_c || !jeu.coup_licite(meilleur_coup)) {
        meilleur_coup = valides[rand_index((int)valides.size())];
    }

    delete racine;

    coup = meilleur_coup;
    _historique_partie.push_back(coup);
}
