#include "starfield.hpp"

#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_snapshot(const starfield::Snapshot& s) {
    std::cout << static_cast<unsigned>(s.state) << ' ' << s.collected_mask << ' ' << std::fixed
              << std::setprecision(4) << s.x << ' ' << s.z << ' ' << s.elapsed
              << ' ' << s.score << '\n';
}

void step(starfield::Game& game, float seconds, float x, float z, bool boost = false) {
    game.update(seconds, {x, z, boost, false});
}

void collect_all(starfield::Game& game) {
    step(game, 0.25f, -1.0f, 0.0f, true);
    step(game, 0.25f, -1.0f, 0.0f, true);
    step(game, 0.25f, -1.0f, 0.0f, true);
    step(game, 0.25f, -1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 0.0f, -1.0f, true);
    step(game, 0.25f, 0.0f, -1.0f, true);
    step(game, 0.25f, 0.0f, -1.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
    step(game, 0.25f, 1.0f, 0.0f, true);
}

} // namespace

int main(int argc, char** argv) {
    starfield::Game game;
    const std::string scenario = argc > 1 ? argv[1] : "startup";
    if (scenario == "startup") {
        // The initial Title snapshot is the startup result.
    } else if (scenario == "collection" || scenario == "win") {
        collect_all(game);
        if (scenario == "win") {
            step(game, 0.25f, -1.0f, 0.0f, true);
            step(game, 0.25f, -1.0f, 0.0f, true);
            step(game, 0.25f, -1.0f, 0.0f, true);
            step(game, 0.25f, 0.0f, -1.0f, true);
            step(game, 0.25f, 0.0f, -1.0f, true);
        }
    } else if (scenario == "hazard") {
        step(game, 0.25f, 0.0f, 1.0f, false);
        step(game, 0.25f, 0.0f, 1.0f, false);
        step(game, 0.25f, 0.0f, 1.0f, false);
    } else if (scenario == "loss") {
        for (int i = 0; i < 241; ++i) {
            step(game, 0.25f, 0.0f, 0.0f);
        }
    } else if (scenario == "restart") {
        step(game, 0.25f, 1.0f, 0.0f);
        game.update(0.0f, {0.0f, 0.0f, false, true});
    } else {
        std::cerr << "unknown scenario: " << scenario << '\n';
        return 2;
    }
    print_snapshot(game.snapshot());
    return 0;
}