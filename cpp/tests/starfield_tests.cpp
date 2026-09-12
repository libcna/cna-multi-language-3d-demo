#include "starfield.hpp"

#include <cmath>
#include <iostream>

namespace {

constexpr float Pi = 3.14159265358979323846f;

void step(starfield::Game& game, float turn, float forward, bool boost = true) {
    starfield::Input input;
    input.turn = turn;
    input.forward = forward;
    input.boost = boost;
    game.update(0.25f, input);
}

void repeat(starfield::Game& game, int count, float turn, float forward) {
    for (int i = 0; i < count; ++i) {
        step(game, turn, forward);
    }
}

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

bool near(float actual, float expected, float tolerance = 0.001f) {
    return std::abs(actual - expected) <= tolerance;
}

} // namespace

int main() {
    bool passed = true;

    starfield::Game movement_game;
    step(movement_game, 1.0f, 0.0f, false);
    passed &= expect(near(movement_game.snapshot().heading, Pi / 4.0f),
                     "turn input must update heading");
    passed &= expect(near(movement_game.snapshot().x, 0.0f) &&
                         near(movement_game.snapshot().z, 0.0f),
                     "turning must not strafe the player");
    step(movement_game, 0.0f, 1.0f, false);
    passed &= expect(near(movement_game.snapshot().x, 0.707107f) &&
                         near(movement_game.snapshot().z, -0.707107f),
                     "forward movement must follow heading");

    starfield::Game hazard_game;
    step(hazard_game, 0.0f, 0.0f, false);
    passed &= expect(near(hazard_game.snapshot().hazard_x, 0.5f) &&
                         near(hazard_game.snapshot().secondary_hazard_x, -0.5f),
                     "snapshot must expose authoritative hazard positions");

    starfield::Game collection_game;
    passed &= expect(starfield::run_scenario(collection_game, "collection"),
                     "collection must be a known scenario");
    const starfield::Snapshot collection = collection_game.snapshot();
    passed &= expect(collection.state == starfield::State::Playing &&
                         collection.collected_mask == 7 && near(collection.score, 300.0f),
                     "collection scenario must collect exactly all three cells");

    starfield::Game win_game;
    passed &= expect(starfield::run_scenario(win_game, "win"),
                     "win must be a known scenario");
    const starfield::Snapshot won = win_game.snapshot();
    passed &= expect(won.state == starfield::State::Won && won.collected_mask == 7 &&
                         near(won.score, 1300.0f) && near(won.x, 0.0f) &&
                         near(won.z, -8.75f),
                     "win scenario must enter the active extraction gate");

    const starfield::Snapshot terminal = win_game.snapshot();
    step(win_game, 1.0f, 1.0f);
    passed &= expect(near(win_game.snapshot().x, terminal.x) &&
                         near(win_game.snapshot().z, terminal.z) &&
                         near(win_game.snapshot().elapsed, terminal.elapsed) &&
                         near(win_game.snapshot().score, terminal.score),
                     "terminal state must freeze simulation and score");

    starfield::Game hazard_collision_game;
    passed &= expect(starfield::run_scenario(hazard_collision_game, "hazard") &&
                         hazard_collision_game.snapshot().state == starfield::State::Lost &&
                         hazard_collision_game.snapshot().elapsed < 60.0f,
                     "hazard scenario must lose by collision before timeout");

    starfield::Game timeout_game;
    passed &= expect(starfield::run_scenario(timeout_game, "loss") &&
                         timeout_game.snapshot().state == starfield::State::Lost &&
                         near(timeout_game.snapshot().elapsed, 60.0f),
                     "loss scenario must lose at the 60 second limit");

    starfield::Input restart;
    restart.restart = true;
    win_game.update(0.0f, restart);
    const starfield::Snapshot reset = win_game.snapshot();
    passed &= expect(reset.state == starfield::State::Title && reset.collected_mask == 0 &&
                         near(reset.x, 0.0f) && near(reset.z, 0.0f) &&
                         near(reset.heading, 0.0f) && near(reset.hazard_x, 0.0f) &&
                         near(reset.elapsed, 0.0f) && near(reset.score, 0.0f),
                     "restart must reset all gameplay state");

    starfield::Game unknown_game;
    passed &= expect(!starfield::run_scenario(unknown_game, "not-a-scenario") &&
                         unknown_game.snapshot().state == starfield::State::Title,
                     "unknown scenario must be rejected without changing state");

    return passed ? 0 : 1;
}
