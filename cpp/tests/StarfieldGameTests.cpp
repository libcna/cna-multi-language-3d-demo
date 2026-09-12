#include "StarfieldGame.hpp"

#include <cmath>
#include <iostream>
#include <type_traits>

namespace starfield
{
    class StarfieldGameTestAccess
    {
    public:
        static void Reset(StarfieldGame& game)
        {
            game.ResetGameplay();
        }

        static void Advance(StarfieldGame& game, float seconds, float turn,
                            float throttle, bool boost = true, bool restart = false)
        {
            game.AdvanceGameplay(seconds, turn, throttle, boost, restart);
        }

        static void PutPlayerAt(StarfieldGame& game, float x, float z)
        {
            game.player_.position.X = x;
            game.player_.position.Z = z;
            game.UpdateCamera();
        }

        static void LoadSector(StarfieldGame& game, int sectorIndex)
        {
            game.LoadSector(sectorIndex);
            game.runState_ = RunState::Playing;
        }
    };
}

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    bool Expect(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << "FAILED: " << message << '\n';
        }
        return condition;
    }

    bool Near(float actual, float expected, float tolerance = 0.001f)
    {
        return std::abs(actual - expected) <= tolerance;
    }

    void Step(starfield::StarfieldGame& game, float turn, float throttle,
              bool boost = true)
    {
        starfield::StarfieldGameTestAccess::Advance(
            game, 0.25f, turn, throttle, boost);
    }

    void HoldFrames(starfield::StarfieldGame& game, int frames, float turn,
                    float throttle, bool boost = false)
    {
        for (int frame = 0; frame < frames; ++frame)
        {
            starfield::StarfieldGameTestAccess::Advance(
                game, 1.0f / 60.0f, turn, throttle, boost);
        }
    }

    void CollectAllAndExtractAtSixtyHz(starfield::StarfieldGame& game)
    {
        // Collect the two Z=0 cells first. Then cross the secondary hazard's
        // Z=-3.5 lane at X=-9, outside its maximum X extent, and approach the
        // center cell along Z=-5.4, outside its collision radius. This proves
        // a continuous 60 Hz route rather than relying on coarse-step tunneling.
        HoldFrames(game, 30, 1.0f, 0.0f);   // face east
        HoldFrames(game, 90, 0.0f, 1.0f);   // collect east cell
        HoldFrames(game, 60, 1.0f, 0.0f);   // face west
        HoldFrames(game, 180, 0.0f, 1.0f);  // collect west cell
        HoldFrames(game, 60, 1.0f, 0.0f);   // face east
        HoldFrames(game, 90, 0.0f, 1.0f);   // return to X=0
        HoldFrames(game, 60, 1.0f, 0.0f);   // face west
        HoldFrames(game, 135, 0.0f, 1.0f);  // move to X=-9
        HoldFrames(game, 30, 1.0f, 0.0f);   // face north
        HoldFrames(game, 81, 0.0f, 1.0f);   // move to Z=-5.4
        HoldFrames(game, 30, 1.0f, 0.0f);   // face east
        HoldFrames(game, 135, 0.0f, 1.0f);  // collect center cell at X=0
        HoldFrames(game, 30, -1.0f, 0.0f);  // face north
        HoldFrames(game, 54, 0.0f, 1.0f);   // enter extraction gate
    }

    void CollectCurrentSectorAtCheckpoints(starfield::StarfieldGame& game)
    {
        const auto collectibles = game.collectibles();
        for (const auto& collectible : collectibles)
        {
            starfield::StarfieldGameTestAccess::PutPlayerAt(
                game, collectible.position.X, collectible.position.Z);
            starfield::StarfieldGameTestAccess::Advance(
                game, 0.0f, 0.0f, 0.0f, false);
        }

        const auto gate = game.extractionPosition();
        starfield::StarfieldGameTestAccess::PutPlayerAt(game, gate.X, gate.Z);
        starfield::StarfieldGameTestAccess::Advance(
            game, 0.0f, 0.0f, 0.0f, false);
    }
}

int main()
{
    static_assert(std::is_base_of_v<Microsoft::Xna::Framework::Game,
                                    starfield::StarfieldGame>);
    static_assert(std::is_final_v<starfield::StarfieldGame>);

    bool passed = true;
    starfield::StarfieldGame game;

    passed &= Expect(game.runState() == starfield::RunState::Title &&
                         game.collectedCount() == 0 && game.score() == 0 &&
                         Near(game.player().position.X, 0.0f) &&
                         Near(game.player().position.Z, 0.0f) &&
                         Near(game.player().heading, 0.0f) &&
                         game.sectorIndex() == 0 &&
                         !game.hazards()[2].active,
                     "initial state must be a complete reset state");

    Step(game, 1.0f, 0.0f, false);
    passed &= Expect(game.runState() == starfield::RunState::Playing &&
                         Near(game.player().heading, Pi / 4.0f) &&
                         Near(game.player().position.X, 0.0f) &&
                         Near(game.player().position.Z, 0.0f),
                     "turning must change heading without strafing");
    Step(game, 0.0f, 1.0f, false);
    passed &= Expect(Near(game.player().position.X, 0.707107f) &&
                         Near(game.player().position.Z, -0.707107f),
                     "forward movement must follow player heading");
    passed &= Expect(Near(game.cameraTarget().X, game.player().position.X) &&
                         Near(game.cameraTarget().Z, game.player().position.Z) &&
                         Near(game.cameraPosition().X,
                              game.player().position.X - std::sin(game.player().heading) * 10.0f) &&
                         Near(game.cameraPosition().Z,
                              game.player().position.Z + std::cos(game.player().heading) * 10.0f),
                     "chase camera must follow position and heading");

    starfield::StarfieldGameTestAccess::Reset(game);
    Step(game, 0.0f, 0.0f, false);
    passed &= Expect(Near(game.hazards()[0].position.X, 0.5f) &&
                         Near(game.hazards()[1].position.X, -0.5f),
                     "both authoritative hazard objects must advance in opposite phases");
    const auto visibleHazardPosition = game.hazards()[0].position;
    starfield::StarfieldGameTestAccess::PutPlayerAt(
        game, visibleHazardPosition.X, visibleHazardPosition.Z);
    starfield::StarfieldGameTestAccess::Advance(game, 0.0f, 0.0f, 0.0f, false);
    passed &= Expect(game.runState() == starfield::RunState::Lost &&
                         game.elapsedSeconds() < 60.0f,
                     "collision must use the same hazard position exposed for drawing");

    starfield::StarfieldGameTestAccess::Reset(game);
    CollectAllAndExtractAtSixtyHz(game);
    if (game.runState() != starfield::RunState::Playing ||
        game.sectorIndex() != 1 || game.collectedCount() != 0 ||
        game.score() != 800)
    {
        std::cerr << "route diagnostic: state=" << static_cast<int>(game.runState())
                  << " sector=" << game.sectorIndex()
                  << " collected=" << game.collectedCount()
                  << " score=" << game.score()
                  << " x=" << game.player().position.X
                  << " z=" << game.player().position.Z
                  << " heading=" << game.player().heading
                  << " hazard0=" << game.hazards()[0].position.X
                  << " hazard1=" << game.hazards()[1].position.X << '\n';
    }
    passed &= Expect(game.runState() == starfield::RunState::Playing &&
                         game.sectorIndex() == 1 && game.collectedCount() == 0 &&
                         game.score() == 800,
                     "a continuous 60 Hz route must clear sector one and advance");

    const auto ionHorizontalBefore = game.hazards()[1].position;
    const auto ionVerticalBefore = game.hazards()[0].position;
    const auto ionOrbiterBefore = game.hazards()[2].position;
    Step(game, 0.0f, 0.0f, false);
    passed &= Expect(game.hazards()[0].motion == starfield::HazardMotion::Vertical &&
                         game.hazards()[0].position.Z > ionVerticalBefore.Z &&
                         game.hazards()[1].motion == starfield::HazardMotion::Horizontal &&
                         game.hazards()[1].position.X < ionHorizontalBefore.X &&
                         game.hazards()[2].motion == starfield::HazardMotion::Orbit &&
                         !Near(game.hazards()[2].position.Z, ionOrbiterBefore.Z),
                     "ion basin hazards must use vertical, horizontal, and orbital motion");

    starfield::StarfieldGameTestAccess::LoadSector(game, 1);
    CollectCurrentSectorAtCheckpoints(game);
    passed &= Expect(game.runState() == starfield::RunState::Playing &&
                         game.sectorIndex() == 2 && game.collectedCount() == 0 &&
                         game.score() == 1600,
                     "clearing sector two must preserve score and load sector three");

    const auto forgeHorizontalBefore = game.hazards()[0].position;
    const auto forgeVerticalBefore = game.hazards()[1].position;
    const auto forgeOrbiterBefore = game.hazards()[2].position;
    Step(game, 0.0f, 0.0f, false);
    passed &= Expect(game.hazards()[0].position.X > forgeHorizontalBefore.X &&
                         game.hazards()[1].position.Z < forgeVerticalBefore.Z &&
                         !Near(game.hazards()[2].position.Z, forgeOrbiterBefore.Z),
                     "solar forge enemies must advance from their authoritative positions");

    starfield::StarfieldGameTestAccess::LoadSector(game, 2);
    CollectCurrentSectorAtCheckpoints(game);
    passed &= Expect(game.runState() == starfield::RunState::Won &&
                         game.sectorIndex() == 2 && game.collectedCount() == 3 &&
                         game.score() == 2900,
                     "clearing all three sectors must enter the final win state");

    const auto wonPosition = game.player().position;
    const float wonTime = game.elapsedSeconds();
    Step(game, 1.0f, 1.0f);
    passed &= Expect(Near(game.player().position.X, wonPosition.X) &&
                         Near(game.player().position.Z, wonPosition.Z) &&
                         Near(game.elapsedSeconds(), wonTime) && game.score() == 2900,
                     "terminal states must freeze gameplay");

    starfield::StarfieldGameTestAccess::Reset(game);
    for (int frame = 0;
         frame < 3700 && game.runState() != starfield::RunState::Lost;
         ++frame)
    {
        starfield::StarfieldGameTestAccess::Advance(
            game, 1.0f / 60.0f, 0.0f, 0.0f, false);
    }
    passed &= Expect(game.runState() == starfield::RunState::Lost &&
                         Near(game.elapsedSeconds(), 60.0f, 0.02f) && game.score() == 0,
                     "the sixty-second timeout must lose without a score bonus");

    starfield::StarfieldGameTestAccess::Advance(
        game, 0.25f, 1.0f, 1.0f, true, true);
    passed &= Expect(game.runState() == starfield::RunState::Title &&
                         game.collectedCount() == 0 && game.score() == 0 &&
                         Near(game.player().position.X, 0.0f) &&
                         Near(game.player().position.Z, 0.0f) &&
                         Near(game.player().heading, 0.0f) &&
                         Near(game.hazards()[0].position.X, 0.0f) &&
                         Near(game.hazards()[1].position.X, 0.0f) &&
                         !game.hazards()[2].active && game.sectorIndex() == 0 &&
                         Near(game.elapsedSeconds(), 0.0f),
                     "restart must reset every authoritative gameplay field");

    return passed ? 0 : 1;
}
