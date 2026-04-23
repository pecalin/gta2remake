#include "core/game.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Game game;

    if (!game.init()) {
        std::fprintf(stderr, "Failed to initialize game\n");
        return 1;
    }

    game.run();
    game.shutdown();

    return 0;
}
