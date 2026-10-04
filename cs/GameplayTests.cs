using System;
using StarfieldCourier;

static void Expect(bool condition, string message)
{
    if (!condition) throw new InvalidOperationException(message);
}
static bool Near(float actual, float expected, float tolerance = 0.001f) => Math.Abs(actual - expected) <= tolerance;
static void Step(StarfieldGame game, float turn, float throttle, bool boost = true) => game.AdvanceGameplay(0.25f, turn, throttle, boost, false);
static void CollectAndExtract(StarfieldGame game)
{
    foreach (var cell in game.Collectibles)
    {
        game.PutPlayerAt(cell.Position.X, cell.Position.Z);
        game.AdvanceGameplay(0f, 0f, 0f, false, false);
    }
    game.PutPlayerAt(game.ExtractionPosition.X, game.ExtractionPosition.Z);
    game.AdvanceGameplay(0f, 0f, 0f, false, false);
}

using var game = new StarfieldGame();
Expect(game.State == RunState.Title && game.Score == 0 && game.SectorIndex == 0, "initial reset");
Step(game, 1f, 0f, false);
Expect(game.State == RunState.Playing && Near(game.Player.Heading, MathF.PI / 4f), "turning");
Step(game, 0f, 1f, false);
Expect(Near(game.Player.Position.X, 0.707107f) && Near(game.Player.Position.Z, -0.707107f), "heading-relative movement");
game.ResetGameplay();
game.LoadSector(1);
game.AdvanceGameplay(0f, 0f, 0f, false, false);
CollectAndExtract(game);
Expect(game.SectorIndex == 2 && game.Score == 800, "sector transition");
CollectAndExtract(game);
Expect(game.State == RunState.Won && game.Score == 2100, "final win from sector two fixture");
var won = game.Player.Position;
Step(game, 1f, 1f);
Expect(game.Player.Position == won, "terminal freeze");
game.ResetGameplay();
for (var i = 0; i < 3700 && game.State != RunState.Lost; i++) game.AdvanceGameplay(1f / 60f, 0f, 0f, false, false);
Expect(game.State == RunState.Lost && Near(game.ElapsedSeconds, 60f, 0.02f), "timeout");
game.AdvanceGameplay(0.25f, 1f, 1f, true, true);
Expect(game.State == RunState.Title && game.Score == 0 && game.SectorIndex == 0, "restart");
Console.WriteLine("starfield-cs gameplay tests passed");
