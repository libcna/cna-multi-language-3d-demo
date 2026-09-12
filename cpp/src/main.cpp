#include "starfield.hpp"

#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_snapshot(const starfield::Snapshot& s) {
    std::cout << static_cast<unsigned>(s.state) << ' ' << s.collected_mask << ' ' << std::fixed
              << std::setprecision(4) << s.x << ' ' << s.z << ' ' << s.elapsed
              << ' ' << s.score << ' ' << s.heading << ' ' << s.hazard_x << ' '
              << s.secondary_hazard_x << '\n';
}

} // namespace

int main(int argc, char** argv) {
    starfield::Game game;
    const std::string scenario = argc > 1 ? argv[1] : "startup";
    if (!starfield::run_scenario(game, scenario)) {
        std::cerr << "unknown scenario: " << scenario << '\n';
        return 2;
    }
    print_snapshot(game.snapshot());
    return 0;
}
