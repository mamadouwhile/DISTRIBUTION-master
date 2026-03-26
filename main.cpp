#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include "arbitre.h"

#define NB_PARTIES 50
using namespace std;

int main(int argc, char* argv[]) {
    std::srand(std::time(nullptr));

    player p1 = player::RAND;
    player p2 = player::MCTS;

    if (argc > 1) {
        std::string mode = argv[1];

        if (mode == "random-mcts") {
            p1 = player::RAND;
            p2 = player::MCTS;
        } else if (mode == "mcts-brutal") {
            p1 = player::MCTS;
            p2 = player::BRUTAL;
        }
    }

    Arbitre a(p1, p2, NB_PARTIES);
    a.challenge();
    return 0;
}
