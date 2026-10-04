using StarfieldCourier;

static RuntimeOptions ParseOptions(string[] args)
{
    var options = new RuntimeOptions();
    for (var i = 0; i < args.Length; i++)
    {
        switch (args[i])
        {
            case "--smoke-frames" when i + 1 < args.Length && int.TryParse(args[++i], out var frames) && frames > 0:
                options.SmokeFrames = frames;
                break;
            case "--screenshot" when i + 1 < args.Length:
                options.ScreenshotPath = args[++i];
                break;
            case "--validate-frame":
                options.ValidateFrame = true;
                break;
            case "--start-sector" when i + 1 < args.Length && int.TryParse(args[++i], out var sector) && sector is >= 1 and <= StarfieldGame.SectorCount:
                options.StartSector = sector - 1;
                break;
            default:
                throw new ArgumentException("usage: starfield_cs [--smoke-frames COUNT] [--validate-frame] [--screenshot FILE.ppm] [--start-sector 1|2|3]");
        }
    }
    if (options.SmokeFrames < 0 && (options.ValidateFrame || options.ScreenshotPath != null)) options.SmokeFrames = 1;
    return options;
}

using var game = new StarfieldGame(ParseOptions(args));
game.Run();
Console.WriteLine($"starfield-cs state={game.State.ToString().ToLowerInvariant()} collected={game.CollectedCount} score={game.Score} sector={game.SectorIndex + 1} frame_valid={game.FrameValid.ToString().ToLowerInvariant()} player_x={game.Player.Position.X} player_z={game.Player.Position.Z} heading={game.Player.Heading}");
Environment.ExitCode = game.FrameValid ? 0 : 1;
