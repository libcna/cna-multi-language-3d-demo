using System.Globalization;

namespace CnaStarfield;

enum State { Title, Playing, Won, Lost }

readonly record struct Input(float X, float Z, bool Boost = false, bool Restart = false);
readonly record struct Snapshot(State State, uint Mask, float X, float Z, float Elapsed, float Score);

sealed class Game
{
    State state = State.Title;
    uint mask;
    float x, z, elapsed, score, hazardX;
    float hazardDirection = 1;

    public Snapshot Snapshot => new(state, mask, x, z, elapsed, score);
    public void Reset() => (state, mask, x, z, elapsed, score, hazardX, hazardDirection) =
        (State.Title, 0, 0, 0, 0, 0, 0, 1);

    public void Update(float seconds, Input input)
    {
        if (input.Restart) { Reset(); return; }
        if (state is State.Won or State.Lost) return;
        if (state == State.Title) state = State.Playing;
        var dt = Math.Clamp(seconds, 0, .25f);
        var speed = input.Boost ? 7 : 4;
        x = Math.Clamp(x + Math.Clamp(input.X, -1, 1) * speed * dt, -10, 10);
        z = Math.Clamp(z + Math.Clamp(input.Z, -1, 1) * speed * dt, -10, 10);
        elapsed += dt;
        hazardX += hazardDirection * 2 * dt;
        if (hazardX >= 7) { hazardX = 7; hazardDirection = -1; }
        if (hazardX <= -7) { hazardX = -7; hazardDirection = 1; }
        float[] cells = {-6, 0, 0, -5, 6, 0};
        for (uint i = 0; i < 3; i++)
            if ((mask & (1u << (int)i)) == 0 && Distance2(x, z, cells[i * 2], cells[i * 2 + 1]) <= .81f)
            { mask |= 1u << (int)i; score += 100; }
        if (Distance2(x, z, hazardX, 3) <= 3.0625f || elapsed >= 60) { state = State.Lost; return; }
        if (mask == 7 && Distance2(x, z, 0, -9) <= 1.96f) { state = State.Won; score += 1000; }
    }

    static float Distance2(float ax, float az, float bx, float bz) =>
        (ax - bx) * (ax - bx) + (az - bz) * (az - bz);
}

static class Program
{
    static void Step(Game game, float x, float z, bool boost = true) => game.Update(.25f, new(x, z, boost));
    static void Collect(Game game)
    {
        for (var i = 0; i < 4; i++) Step(game, -1, 0);
        for (var i = 0; i < 4; i++) Step(game, 1, 0);
        for (var i = 0; i < 3; i++) Step(game, 0, -1);
        for (var i = 0; i < 4; i++) Step(game, 1, 0);
    }
    static void Print(Snapshot s) => Console.WriteLine(string.Join(" ", (uint)s.State, s.Mask,
        s.X.ToString("F4", CultureInfo.InvariantCulture), s.Z.ToString("F4", CultureInfo.InvariantCulture),
        s.Elapsed.ToString("F4", CultureInfo.InvariantCulture), s.Score.ToString("F4", CultureInfo.InvariantCulture)));
    static int Main(string[] args)
    {
        var game = new Game();
        var scenario = args.Length == 0 ? "startup" : args[0];
        if (scenario is "collection" or "win") { Collect(game); if (scenario == "win") { for (var i = 0; i < 3; i++) Step(game, -1, 0); for (var i = 0; i < 2; i++) Step(game, 0, -1); } }
        else if (scenario == "hazard") { Step(game, 0, 1, false); Step(game, 0, 1, false); Step(game, 0, 1, false); }
        else if (scenario == "loss") for (var i = 0; i < 241; i++) Step(game, 0, 0, false);
        else if (scenario == "restart") { Step(game, 1, 0); game.Update(0, new(0, 0, false, true)); }
        else if (scenario != "startup") return 2;
        Print(game.Snapshot);
        return 0;
    }
}