using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Audio;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework.Media;

namespace StarfieldCourier;

internal enum RunState
{
    Title,
    Playing,
    Won,
    Lost
}

internal enum HazardMotion
{
    Horizontal,
    Vertical,
    Orbit
}

internal sealed class Player
{
    internal Vector3 Position;
    internal float Heading;
}

internal sealed class Hazard
{
    internal Vector3 Position;
    internal Vector3 Origin;
    internal Vector3 Velocity;
    internal HazardMotion Motion;
    internal float Phase;
    internal float Range;
    internal bool Active;
}

internal sealed class Collectible
{
    internal Vector3 Position;
    internal bool Collected;
}

internal sealed class RuntimeOptions
{
    internal int SmokeFrames { get; set; } = -1;
    internal string? ScreenshotPath { get; set; }
    internal string ContentRoot { get; set; } = Path.Combine(AppContext.BaseDirectory, "Content");
    internal int StartSector { get; set; }
    internal bool ValidateFrame { get; set; }
}

internal sealed class StarfieldGame : Game
{
    internal const int ReferenceWidth = 1280;
    internal const int ReferenceHeight = 720;
    internal const int SectorCount = 3;

    private const float Pi = 3.14159265358979323846f;
    private const float ArenaLimit = 10f;
    private const float TurnSpeed = Pi;
    private const float NormalSpeed = 4f;
    private const float BoostSpeed = 7f;
    private const float HazardSpeed = 2f;
    private const float CellRadius = 0.9f;
    private const float HazardCollisionRadius = 1.75f;
    private const float ExtractionRadius = 1.4f;
    private const float TimeLimit = 60f;

    private static readonly byte[] DigitSegments =
    {
        0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
        0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011
    };

    private readonly RuntimeOptions options;
    private readonly GraphicsDeviceManager graphics;
    private readonly Player player = new();
    private readonly Hazard[] hazards = { new(), new(), new() };
    private readonly Collectible[] collectibles = { new(), new(), new() };
    private RunState runState = RunState.Title;
    private Vector3 cameraPosition;
    private Vector3 cameraTarget;
    private Vector3 extractionPosition;
    private float elapsedSeconds;
    private int score;
    private int sectorIndex;
    private int drawnFrames;
    private float presentationSeconds;
    private float sectorBannerSeconds;
    private bool restartHeld;
    private bool captured;
    private bool frameValid = true;
    private BasicEffect? effect;
    private RenderTarget2D? captureTarget;
    private SoundEffect? collectSound;
    private SoundEffect? lossSound;
    private SoundEffect? restartSound;
    private SoundEffect? winSound;
    private Song? backgroundMusic;

    internal StarfieldGame(RuntimeOptions? options = null)
    {
        this.options = options ?? new RuntimeOptions();
        graphics = new GraphicsDeviceManager(this)
        {
            PreferredBackBufferWidth = ReferenceWidth,
            PreferredBackBufferHeight = ReferenceHeight,
            PreferredDepthStencilFormat = DepthFormat.Depth24
        };
        IsFixedTimeStep = true;
        TargetElapsedTime = TimeSpan.FromMilliseconds(1000.0 / 60.0);
        IsMouseVisible = true;
        Window.Title = "Starfield Courier — C#";
        ResetGameplay();
    }

    internal RunState State => runState;
    internal Player Player => player;
    internal IReadOnlyList<Hazard> Hazards => hazards;
    internal IReadOnlyList<Collectible> Collectibles => collectibles;
    internal Vector3 CameraPosition => cameraPosition;
    internal Vector3 CameraTarget => cameraTarget;
    internal Vector3 ExtractionPosition => extractionPosition;
    internal float ElapsedSeconds => elapsedSeconds;
    internal int Score => score;
    internal int SectorIndex => sectorIndex;
    internal bool FrameValid => frameValid;
    internal int CollectedCount
    {
        get
        {
            var result = 0;
            foreach (var collectible in collectibles)
            {
                if (collectible.Collected)
                {
                    result++;
                }
            }
            return result;
        }
    }

    protected override void Initialize()
    {
        ResetGameplay();
        if (options.StartSector > 0)
        {
            LoadSector(Math.Clamp(options.StartSector, 0, SectorCount - 1));
            runState = RunState.Title;
            score = 0;
        }
        base.Initialize();
    }

    protected override void LoadContent()
    {
        effect = new BasicEffect(GraphicsDevice) { VertexColorEnabled = true };
        if (!string.IsNullOrEmpty(options.ScreenshotPath) || options.ValidateFrame)
        {
            captureTarget = new RenderTarget2D(
                GraphicsDevice, ReferenceWidth, ReferenceHeight, false,
                SurfaceFormat.Color, DepthFormat.Depth24, 0,
                RenderTargetUsage.PreserveContents);
        }

        collectSound = LoadSound("collect.wav");
        lossSound = LoadSound("loss.wav");
        restartSound = LoadSound("restart.wav");
        winSound = LoadSound("win.wav");
        var musicPath = Path.GetFullPath(Path.Combine(options.ContentRoot, "audio", "outer_space.mp3"));
        backgroundMusic = Song.FromUri("Outer Space Loop", new Uri(musicPath));
        MediaPlayer.Volume = 0.14f;
        MediaPlayer.IsRepeating = true;
        MediaPlayer.Play(backgroundMusic);
    }

    protected override void UnloadContent()
    {
        MediaPlayer.Stop();
        backgroundMusic?.Dispose();
        winSound?.Dispose();
        restartSound?.Dispose();
        lossSound?.Dispose();
        collectSound?.Dispose();
        captureTarget?.Dispose();
        effect?.Dispose();
        backgroundMusic = null;
        winSound = restartSound = lossSound = collectSound = null;
        captureTarget = null;
        effect = null;
        base.UnloadContent();
    }

    protected override void Update(GameTime gameTime)
    {
        presentationSeconds = (float)gameTime.TotalGameTime.TotalSeconds;
        var keyboard = Keyboard.GetState();
        if (keyboard.IsKeyDown(Keys.Escape))
        {
            Exit();
            return;
        }

        var turn = (keyboard.IsKeyDown(Keys.D) || keyboard.IsKeyDown(Keys.Right) ? 1f : 0f) -
                   (keyboard.IsKeyDown(Keys.A) || keyboard.IsKeyDown(Keys.Left) ? 1f : 0f);
        var throttle = (keyboard.IsKeyDown(Keys.W) || keyboard.IsKeyDown(Keys.Up) ? 1f : 0f) -
                       (keyboard.IsKeyDown(Keys.S) || keyboard.IsKeyDown(Keys.Down) ? 1f : 0f);
        var restartDown = keyboard.IsKeyDown(Keys.R) || keyboard.IsKeyDown(Keys.Enter);
        var restart = restartDown && !restartHeld;
        restartHeld = restartDown;
        AdvanceGameplay(
            (float)gameTime.ElapsedGameTime.TotalSeconds, turn, throttle,
            keyboard.IsKeyDown(Keys.Space), restart);
        base.Update(gameTime);
    }

    protected override void Draw(GameTime gameTime)
    {
        var captureThisFrame = captureTarget != null && !captured;
        if (captureThisFrame)
        {
            GraphicsDevice.SetRenderTarget(captureTarget);
        }

        GraphicsDevice.BlendState = BlendState.Opaque;
        GraphicsDevice.Clear(ThemeForSector(sectorIndex).Sky);
        GraphicsDevice.RasterizerState = RasterizerState.CullNone;
        DrawWorld();
        DrawHud();

        if (captureThisFrame)
        {
            GraphicsDevice.SetRenderTarget(null);
            CaptureFrame();
            captured = true;
        }

        base.Draw(gameTime);
        drawnFrames++;
        if (options.SmokeFrames > 0 && drawnFrames >= options.SmokeFrames)
        {
            Exit();
        }
    }

    internal void ResetGameplay()
    {
        runState = RunState.Title;
        score = 0;
        LoadSector(0);
    }

    internal void LoadSector(int index)
    {
        sectorIndex = Math.Clamp(index, 0, SectorCount - 1);
        elapsedSeconds = 0f;
        sectorBannerSeconds = 2.4f;
        for (var i = 0; i < hazards.Length; i++)
        {
            hazards[i] = new Hazard();
            collectibles[i] = new Collectible();
        }

        switch (sectorIndex)
        {
            case 1:
                SetPlayer(0f, 8f);
                extractionPosition = new Vector3(8f, 0f, -8f);
                SetCell(0, -7f, 6f); SetCell(1, 7f, -1f); SetCell(2, -5f, -6f);
                SetHazard(0, -2f, 0.8f, 0f, 0f, 2f, HazardMotion.Vertical, 7f);
                SetHazard(1, 0f, 0.7f, -2f, -2f, 0f, HazardMotion.Horizontal, 7f);
                SetOrbiter(2, 2f, 0.9f, 2f, 2.5f);
                break;
            case 2:
                SetPlayer(0f, 8f);
                extractionPosition = new Vector3(-8f, 0f, -8f);
                SetCell(0, -7f, -5f); SetCell(1, 0f, -6f); SetCell(2, 6f, 6f);
                SetHazard(0, 0f, 0.8f, 4f, 2.5f, 0f, HazardMotion.Horizontal, 8f);
                SetHazard(1, -4f, 0.7f, 0f, 0f, -2.4f, HazardMotion.Vertical, 8f);
                SetOrbiter(2, 3f, 1f, -3f, 3f);
                break;
            default:
                SetPlayer(0f, 0f);
                extractionPosition = new Vector3(0f, 0f, -9f);
                SetCell(0, -6f, 0f); SetCell(1, 0f, -5f); SetCell(2, 6f, 0f);
                SetHazard(0, 0f, 0.8f, 3f, 2f, 0f, HazardMotion.Horizontal, 7f);
                SetHazard(1, 0f, 0.6f, -3.5f, -2f, 0f, HazardMotion.Horizontal, 7f);
                break;
        }
        UpdateCamera();
    }

    internal void AdvanceGameplay(float seconds, float turn, float throttle, bool boost, bool restart)
    {
        if (restart)
        {
            PlaySound(restartSound, 0.42f, 0.08f);
            ResetGameplay();
            return;
        }
        if (runState is RunState.Won or RunState.Lost)
        {
            return;
        }
        if (runState == RunState.Title)
        {
            runState = RunState.Playing;
        }

        var dt = Math.Clamp(seconds, 0f, 0.25f);
        sectorBannerSeconds = Math.Max(0f, sectorBannerSeconds - dt);
        player.Heading = MathF.IEEERemainder(
            player.Heading + Math.Clamp(turn, -1f, 1f) * TurnSpeed * dt, 2f * Pi);
        var movement = Math.Clamp(throttle, -1f, 1f) * (boost ? BoostSpeed : NormalSpeed) * dt;
        player.Position.X = Math.Clamp(player.Position.X + MathF.Sin(player.Heading) * movement,
            -ArenaLimit, ArenaLimit);
        player.Position.Z = Math.Clamp(player.Position.Z - MathF.Cos(player.Heading) * movement,
            -ArenaLimit, ArenaLimit);
        elapsedSeconds += dt;

        foreach (var hazard in hazards)
        {
            if (hazard.Active)
            {
                MoveHazard(hazard, dt);
            }
        }

        var collectedThisStep = false;
        foreach (var collectible in collectibles)
        {
            if (!collectible.Collected && DistanceSquaredXZ(player.Position, collectible.Position) <= CellRadius * CellRadius)
            {
                collectible.Collected = true;
                score += 100;
                collectedThisStep = true;
            }
        }

        foreach (var hazard in hazards)
        {
            if (hazard.Active && DistanceSquaredXZ(player.Position, hazard.Position) <= HazardCollisionRadius * HazardCollisionRadius)
            {
                runState = RunState.Lost;
                PlaySound(lossSound, 0.62f, -0.18f);
                UpdateCamera();
                return;
            }
        }
        if (elapsedSeconds >= TimeLimit)
        {
            runState = RunState.Lost;
            PlaySound(lossSound, 0.62f, -0.35f);
            UpdateCamera();
            return;
        }

        if (CollectedCount == collectibles.Length &&
            DistanceSquaredXZ(player.Position, extractionPosition) <= ExtractionRadius * ExtractionRadius)
        {
            if (sectorIndex + 1 < SectorCount)
            {
                score += 500;
                LoadSector(sectorIndex + 1);
                PlaySound(winSound, 0.48f, -0.08f);
                return;
            }
            runState = RunState.Won;
            score += 1000;
            PlaySound(winSound, 0.62f, 0.08f);
        }
        else if (collectedThisStep)
        {
            PlaySound(collectSound, 0.52f, -0.08f + CollectedCount * 0.08f);
        }
        UpdateCamera();
    }

    internal void PutPlayerAt(float x, float z)
    {
        player.Position.X = x;
        player.Position.Z = z;
        UpdateCamera();
    }

    private void SetPlayer(float x, float z)
    {
        player.Position = new Vector3(x, 0.6f, z);
        player.Heading = 0f;
    }

    private void SetCell(int index, float x, float z)
    {
        collectibles[index].Position = new Vector3(x, 0.7f, z);
    }

    private void SetHazard(int index, float x, float y, float z, float velocityX, float velocityZ,
        HazardMotion motion, float range)
    {
        hazards[index].Position = hazards[index].Origin = new Vector3(x, y, z);
        hazards[index].Velocity = new Vector3(velocityX, 0f, velocityZ);
        hazards[index].Motion = motion;
        hazards[index].Range = range;
        hazards[index].Active = true;
    }

    private void SetOrbiter(int index, float x, float y, float z, float range)
    {
        SetHazard(index, x + range, y, z, 0f, 0f, HazardMotion.Orbit, range);
        hazards[index].Origin = new Vector3(x, y, z);
    }

    private void MoveHazard(Hazard hazard, float seconds)
    {
        if (hazard.Motion == HazardMotion.Orbit)
        {
            hazard.Phase = MathF.IEEERemainder(hazard.Phase + seconds * 1.35f, 2f * Pi);
            hazard.Position.X = hazard.Origin.X + MathF.Cos(hazard.Phase) * hazard.Range;
            hazard.Position.Z = hazard.Origin.Z + MathF.Sin(hazard.Phase) * hazard.Range;
            return;
        }

        var horizontal = hazard.Motion == HazardMotion.Horizontal;
        var coordinate = horizontal ? hazard.Position.X : hazard.Position.Z;
        var speed = horizontal ? hazard.Velocity.X : hazard.Velocity.Z;
        var center = horizontal ? hazard.Origin.X : hazard.Origin.Z;
        coordinate += speed * seconds;
        if (coordinate >= center + hazard.Range)
        {
            coordinate = center + hazard.Range;
            speed = -Math.Abs(speed);
        }
        else if (coordinate <= center - hazard.Range)
        {
            coordinate = center - hazard.Range;
            speed = Math.Abs(speed);
        }
        if (horizontal)
        {
            hazard.Position.X = coordinate;
            hazard.Velocity.X = speed;
        }
        else
        {
            hazard.Position.Z = coordinate;
            hazard.Velocity.Z = speed;
        }
    }

    private void UpdateCamera()
    {
        var forwardX = MathF.Sin(player.Heading);
        var forwardZ = -MathF.Cos(player.Heading);
        cameraPosition = new Vector3(player.Position.X - forwardX * 10f, 7f,
            player.Position.Z - forwardZ * 10f);
        cameraTarget = new Vector3(player.Position.X, 0.5f, player.Position.Z);
    }

    private void DrawWorld()
    {
        var fx = effect!;
        GraphicsDevice.DepthStencilState = DepthStencilState.Default;
        GraphicsDevice.BlendState = BlendState.Opaque;
        fx.World = Matrix.Identity;
        fx.View = Matrix.CreateLookAt(cameraPosition, cameraTarget, Vector3.Up);
        fx.Projection = Matrix.CreatePerspectiveFieldOfView(Pi / 4f, 16f / 9f, 0.1f, 100f);
        DrawStarfield();
        DrawArena();
        DrawPlayerCraft();
        for (var i = 0; i < collectibles.Length; i++)
        {
            if (!collectibles[i].Collected) DrawCollectible(collectibles[i], i);
            if (hazards[i].Active) DrawHazard(hazards[i], i);
        }
        DrawExtractionGate();
    }

    private void DrawStarfield()
    {
        var theme = ThemeForSector(sectorIndex);
        for (var i = 0; i < 52; i++)
        {
            var angle = i * 2.3999632f;
            var radius = 25f + i * 7 % 11;
            var height = 3f + i * 13 % 17;
            var size = 0.05f + i % 4 * 0.025f;
            DrawCube(new Vector3(MathF.Cos(angle) * radius, height, MathF.Sin(angle) * radius),
                new Vector3(size), i % 7 == 0 ? theme.Accent : new Color(190, 210, 235));
        }
    }

    private void DrawArena()
    {
        var theme = ThemeForSector(sectorIndex);
        DrawCube(new Vector3(0f, -0.38f, 0f), new Vector3(21f, 0.6f, 21f), Shade(theme.Floor, 0.38f));
        DrawCube(new Vector3(0f, -0.055f, 0f), new Vector3(20f, 0.08f, 20f), theme.Floor);
        for (var offset = -10; offset <= 10; offset += 2)
        {
            DrawCube(new Vector3(offset, 0.005f, 0f), new Vector3(0.028f, 0.025f, 20f), theme.Grid);
            DrawCube(new Vector3(0f, 0.005f, offset), new Vector3(20f, 0.025f, 0.028f), theme.Grid);
        }
        DrawCube(new Vector3(0f, 0.022f, 0f), new Vector3(0.06f, 0.035f, 20f), Shade(theme.Accent, 0.72f));
        DrawCube(new Vector3(extractionPosition.X, 0.024f, extractionPosition.Z),
            new Vector3(3.8f, 0.04f, 1.7f), new Color(15, 68, 64));

        if (sectorIndex == 0)
        {
            foreach (var laneX in new[] { -1.35f, 1.35f })
                DrawCube(new Vector3(laneX, 0.05f, -7f), new Vector3(0.06f, 0.05f, 5.4f), new Color(35, 155, 110));
        }
        else if (sectorIndex == 1)
        {
            foreach (var diagonal in new[] { -5f, 0f, 5f })
                DrawCube(new Vector3(diagonal, 0.035f, diagonal), new Vector3(0.1f, 0.035f, 13.5f),
                    Shade(theme.Accent, 0.7f), -Pi / 4f);
            foreach (var side in new[] { -1f, 1f })
            {
                DrawCube(new Vector3(side * 11.2f, 2f, -6f), new Vector3(0.7f, 4f, 0.7f), theme.Accent, side * 0.25f);
                DrawCube(new Vector3(side * 11.4f, 1.2f, 5.5f), new Vector3(1f, 2.4f, 1f), theme.Boundary, -side * 0.35f);
            }
        }
        else
        {
            foreach (var channelX in new[] { -6f, -2f, 2f, 6f })
                DrawCube(new Vector3(channelX, 0.035f, 0f), new Vector3(0.34f, 0.035f, 19.6f),
                    channelX is -2f or 2f ? theme.Accent : Shade(theme.Boundary, 0.7f));
            foreach (var side in new[] { -1f, 1f })
            foreach (var z in new[] { -6f, 0f, 6f })
            {
                DrawCube(new Vector3(side * 11.35f, 1.45f, z), new Vector3(1.2f, 2.9f, 1.2f), new Color(75, 20, 18));
                DrawCube(new Vector3(side * 11.35f, 3f, z), new Vector3(0.75f, 0.25f, 0.75f), theme.Boundary);
            }
        }

        DrawCube(new Vector3(-10.1f, 0.18f, 0f), new Vector3(0.22f, 0.36f, 20.4f), theme.Boundary);
        DrawCube(new Vector3(10.1f, 0.18f, 0f), new Vector3(0.22f, 0.36f, 20.4f), theme.Boundary);
        DrawCube(new Vector3(0f, 0.18f, -10.1f), new Vector3(20.4f, 0.36f, 0.22f), theme.Boundary);
        DrawCube(new Vector3(0f, 0.18f, 10.1f), new Vector3(20.4f, 0.36f, 0.22f), theme.Boundary);
        foreach (var edge in new[] { -10f, 10f })
        foreach (var offset in new[] { -10f, -5f, 0f, 5f, 10f })
        {
            DrawCube(new Vector3(edge, 0.8f, offset), new Vector3(0.32f, 1.6f, 0.32f), Shade(theme.Boundary, 0.38f));
            DrawCube(new Vector3(edge, 1.62f, offset), new Vector3(0.46f, 0.12f, 0.46f), theme.Accent);
            DrawCube(new Vector3(offset, 0.8f, edge), new Vector3(0.32f, 1.6f, 0.32f), Shade(theme.Boundary, 0.38f));
            DrawCube(new Vector3(offset, 1.62f, edge), new Vector3(0.46f, 0.12f, 0.46f), theme.Accent);
        }
    }

    private void DrawPlayerCraft()
    {
        var yaw = -player.Heading;
        DrawCube(player.Position, new Vector3(1.2f, 0.6f, 1.8f), new Color(0, 175, 225), yaw);
        DrawCube(LocalOffset(player.Position, player.Heading, 0f, 0.34f, 0.2f), new Vector3(0.62f, 0.28f, 0.8f), new Color(25, 60, 105), yaw);
        DrawCube(LocalOffset(player.Position, player.Heading, 0f, 0.02f, 1.05f), new Vector3(0.52f, 0.32f, 0.65f), new Color(100, 245, 255), yaw);
        foreach (var side in new[] { -1f, 1f })
        {
            DrawCube(LocalOffset(player.Position, player.Heading, side * 0.86f, -0.08f, -0.05f), new Vector3(0.95f, 0.14f, 0.9f), new Color(0, 115, 175), yaw);
            DrawCube(LocalOffset(player.Position, player.Heading, side * 0.48f, -0.02f, -0.93f), new Vector3(0.28f, 0.32f, 0.48f), new Color(25, 65, 110), yaw);
            DrawCube(LocalOffset(player.Position, player.Heading, side * 0.48f, -0.02f, -1.2f), new Vector3(0.17f, 0.2f, 0.25f), new Color(80, 220, 255), yaw);
        }
    }

    private void DrawCollectible(Collectible collectible, int index)
    {
        var phase = presentationSeconds * 1.8f + index * 1.7f;
        var pulse = 1f + MathF.Sin(phase) * 0.08f;
        var center = collectible.Position;
        DrawCube(new Vector3(center.X, 0.14f, center.Z), new Vector3(1.2f, 0.18f, 1.2f), new Color(75, 55, 12), phase * 0.7f);
        DrawCube(center, new Vector3(0.52f * pulse, 1.2f, 0.52f * pulse), new Color(255, 210, 0), phase * 0.7f);
        DrawCube(center, new Vector3(1.05f, 0.09f, 0.16f), new Color(255, 242, 125), phase * 0.7f);
        DrawCube(center, new Vector3(0.16f, 0.09f, 1.05f), new Color(255, 242, 125), phase * 0.7f);
    }

    private void DrawHazard(Hazard hazard, int index)
    {
        var spin = presentationSeconds * 1.6f * (index == 0 ? 1f : -1f);
        var pulse = 1f + 0.1f * MathF.Sin(presentationSeconds * 4f + index);
        var color = hazard.Motion switch
        {
            HazardMotion.Vertical => new Color(205, 45, 235),
            HazardMotion.Orbit => new Color(255, 105, 25),
            _ => new Color(230, 40, 50)
        };
        DrawCube(hazard.Position, new Vector3(1.15f * pulse, 1.05f, 1.15f * pulse), color, spin);
        DrawCube(hazard.Position, new Vector3(2.2f, 0.2f, 0.34f), Shade(color, 0.58f), spin);
        DrawCube(hazard.Position, new Vector3(0.34f, 0.2f, 2.2f), Shade(color, 0.58f), spin);
    }

    private void DrawExtractionGate()
    {
        var active = CollectedCount == collectibles.Length;
        var gateColor = active ? new Color(40, 255, 80) : new Color(20, 100, 50);
        var trim = active ? new Color(145, 255, 190) : new Color(35, 135, 85);
        var x = extractionPosition.X;
        var z = extractionPosition.Z;
        DrawCube(new Vector3(x, 0.05f, z), new Vector3(3.7f, 0.1f, 1.3f), new Color(12, 70, 58));
        foreach (var side in new[] { -1.45f, 1.45f })
        {
            DrawCube(new Vector3(x + side, 1.05f, z), new Vector3(0.42f, 2.1f, 0.62f), new Color(10, 58, 52));
            DrawCube(new Vector3(x + side, 1.15f, z), new Vector3(0.2f, 1.65f, 0.7f), gateColor);
            DrawCube(new Vector3(x + side, 2.18f, z), new Vector3(0.56f, 0.18f, 0.78f), trim);
        }
        DrawCube(new Vector3(x, 2.25f, z), new Vector3(3.3f, 0.35f, 0.62f), new Color(10, 58, 52));
        DrawCube(new Vector3(x, 2.23f, z), new Vector3(2.72f, 0.16f, 0.72f), gateColor);
    }

    private void DrawHud()
    {
        var fx = effect!;
        var theme = ThemeForSector(sectorIndex);
        GraphicsDevice.DepthStencilState = DepthStencilState.None;
        GraphicsDevice.BlendState = BlendState.AlphaBlend;
        fx.View = Matrix.Identity;
        fx.Projection = Matrix.CreateOrthographicOffCenter(0f, ReferenceWidth, ReferenceHeight, 0f, 0f, 10f);
        DrawHudRect(22f, 18f, 435f, 88f, new Color(5, 12, 30, 225));
        DrawText("CELLS", 36f, 29f, 2f, new Color(130, 205, 255));
        for (var i = 0; i < collectibles.Length; i++)
            DrawHudRect(36f + i * 38f, 60f, 27f, 27f, collectibles[i].Collected ? new Color(255, 215, 0) : new Color(35, 52, 78));
        DrawText("TIME", 184f, 29f, 2f, new Color(130, 205, 255));
        DrawHudRect(184f, 66f, 244f, 14f, new Color(24, 38, 63));
        var remaining = Math.Max(0f, TimeLimit - elapsedSeconds) / TimeLimit;
        DrawHudRect(187f, 69f, remaining * 238f, 8f, remaining > 0.25f ? new Color(60, 230, 130) : new Color(255, 80, 50));
        DrawHudRect(474f, 18f, 500f, 56f, new Color(5, 12, 30, 210));
        DrawText(runState == RunState.Won ? "MISSION COMPLETE" : runState == RunState.Lost ? "MISSION FAILED" : $"SECTOR {sectorIndex + 1} {SectorName(sectorIndex)}",
            506f, 36f, 2f, theme.Accent);
        DrawHudRect(996f, 18f, 262f, 88f, new Color(5, 12, 30, 225));
        DrawText("SCORE", 1011f, 29f, 2f, new Color(130, 205, 255));
        var displayScore = score;
        for (var digit = 3; digit >= 0; digit--)
        {
            DrawDigit(displayScore % 10, 1230f - (3 - digit) * 34f, 68f, new Color(100, 245, 255));
            displayScore /= 10;
        }
        DrawHudRect(22f, 682f, 610f, 25f, new Color(5, 12, 30, 205));
        DrawText("WASD MOVE  R RESTART  ESC QUIT", 34f, 688f, 1.5f, new Color(115, 165, 205));

        if (sectorBannerSeconds > 0f && runState is not (RunState.Won or RunState.Lost))
        {
            DrawHudRect(350f, 530f, 580f, 122f, new Color(3, 8, 24, 145));
            var title = $"SECTOR {sectorIndex + 1}";
            var name = SectorName(sectorIndex);
            DrawText(title, CenteredX(title.Length, 3f), 553f, 3f, Color.White);
            DrawText(name, CenteredX(name.Length, 4f), 600f, 4f, theme.Accent);
        }
        if (runState is RunState.Won or RunState.Lost)
        {
            var terminal = runState == RunState.Won ? new Color(40, 255, 90) : new Color(255, 70, 55);
            DrawHudRect(145f, 205f, 990f, 305f, new Color(3, 8, 24, 238));
            var headline = runState == RunState.Won ? "MISSION COMPLETE" : "MISSION FAILED";
            DrawText(headline, CenteredX(headline.Length, 5f), 258f, 5f, terminal);
            const string prompt = "PRESS R OR ENTER TO RESTART";
            DrawText(prompt, CenteredX(prompt.Length, 3f), 423f, 3f, Color.White);
        }
        GraphicsDevice.BlendState = BlendState.Opaque;
    }

    private void DrawCube(Vector3 position, Vector3 scale, Color color, float yaw = 0f)
    {
        var vertices = MakeCube(color);
        effect!.World = Matrix.CreateScale(scale) * Matrix.CreateRotationY(yaw) * Matrix.CreateTranslation(position);
        effect.CurrentTechnique.Passes[0].Apply();
        GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, vertices, 0, vertices.Length / 3);
    }

    private void DrawHudRect(float x, float y, float width, float height, Color color) =>
        DrawCube(new Vector3(x + width * 0.5f, y + height * 0.5f, 0f), new Vector3(width, height, 0.1f), color);

    private void DrawText(string text, float x, float y, float pixelSize, Color color)
    {
        for (var characterIndex = 0; characterIndex < text.Length; characterIndex++)
        {
            var rows = GlyphRows(text[characterIndex]);
            for (var row = 0; row < rows.Length; row++)
            for (var column = 0; column < 5; column++)
                if ((rows[row] & 1 << (4 - column)) != 0)
                    DrawHudRect(x + (characterIndex * 6f + column) * pixelSize,
                        y + row * pixelSize, pixelSize, pixelSize, color);
        }
    }

    private void DrawDigit(int digit, float x, float y, Color color)
    {
        var segments = DigitSegments[digit];
        if ((segments & 0b1000000) != 0) DrawCube(new Vector3(x, y - 18f, 0f), new Vector3(16f, 4f, 0.1f), color);
        if ((segments & 0b0100000) != 0) DrawCube(new Vector3(x + 8f, y - 9f, 0f), new Vector3(4f, 16f, 0.1f), color);
        if ((segments & 0b0010000) != 0) DrawCube(new Vector3(x + 8f, y + 9f, 0f), new Vector3(4f, 16f, 0.1f), color);
        if ((segments & 0b0001000) != 0) DrawCube(new Vector3(x, y + 18f, 0f), new Vector3(16f, 4f, 0.1f), color);
        if ((segments & 0b0000100) != 0) DrawCube(new Vector3(x - 8f, y + 9f, 0f), new Vector3(4f, 16f, 0.1f), color);
        if ((segments & 0b0000010) != 0) DrawCube(new Vector3(x - 8f, y - 9f, 0f), new Vector3(4f, 16f, 0.1f), color);
        if ((segments & 0b0000001) != 0) DrawCube(new Vector3(x, y, 0f), new Vector3(16f, 4f, 0.1f), color);
    }

    private SoundEffect LoadSound(string name)
    {
        using var stream = File.OpenRead(Path.Combine(options.ContentRoot, "audio", name));
        return SoundEffect.FromStream(stream);
    }

    private static void PlaySound(SoundEffect? sound, float volume, float pitch)
    {
        try { sound?.Play(volume, pitch, 0f); } catch { }
    }

    private void CaptureFrame()
    {
        var target = captureTarget!;
        var pixels = new Color[target.Width * target.Height];
        target.GetData(pixels);
        var theme = ThemeForSector(sectorIndex);
        long nonClear = 0, floor = 0, grid = 0, boundary = 0, playerPixels = 0,
            collectiblePixels = 0, hazardPixels = 0, gatePixels = 0, hud = 0;
        for (var i = 0; i < pixels.Length; i++)
        {
            var pixel = pixels[i];
            if (pixel != theme.Sky) nonClear++;
            if (pixel == theme.Floor) floor++;
            if (pixel == theme.Grid) grid++;
            if (pixel == theme.Boundary) boundary++;
            if (pixel.B > 160 && pixel.G > 100 && pixel.R < 120) playerPixels++;
            if (pixel.R > 210 && pixel.G > 135 && pixel.B < 130) collectiblePixels++;
            if (pixel.R > 175 && pixel.G < 100 && pixel.B < 110) hazardPixels++;
            if (pixel.G > 50 && pixel.G > pixel.R * 2 && pixel.G > pixel.B) gatePixels++;
            if (i < target.Width * 115 && pixel != theme.Sky) hud++;
        }
        frameValid = target.Width == ReferenceWidth && target.Height == ReferenceHeight &&
            nonClear > pixels.Length / 100 && floor > pixels.Length / 10 && grid > 2500 &&
            boundary > 3000 && playerPixels > 500 && collectiblePixels > 500 &&
            hazardPixels > 250 && gatePixels > 300 && hud > 5000;
        if (options.ValidateFrame && !frameValid)
            throw new InvalidOperationException($"incomplete frame: non-clear={nonClear} floor={floor} grid={grid} boundary={boundary} player={playerPixels} collectible={collectiblePixels} hazard={hazardPixels} gate={gatePixels} hud={hud}");

        if (!string.IsNullOrEmpty(options.ScreenshotPath))
        {
            using var output = File.Create(options.ScreenshotPath);
            using var writer = new BinaryWriter(output);
            writer.Write(System.Text.Encoding.ASCII.GetBytes($"P6\n{target.Width} {target.Height}\n255\n"));
            foreach (var pixel in pixels) { writer.Write(pixel.R); writer.Write(pixel.G); writer.Write(pixel.B); }
        }
    }

    private static VertexPositionColor[] MakeCube(Color color)
    {
        var points = new[]
        {
            new Vector3(-.5f,-.5f,-.5f), new Vector3(.5f,-.5f,-.5f), new Vector3(.5f,.5f,-.5f), new Vector3(-.5f,.5f,-.5f),
            new Vector3(-.5f,-.5f,.5f), new Vector3(.5f,-.5f,.5f), new Vector3(.5f,.5f,.5f), new Vector3(-.5f,.5f,.5f)
        };
        int[] indices = { 0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 3,2,6,3,6,7, 1,5,6,1,6,2, 0,3,7,0,7,4 };
        float[] brightness = { .82f, .58f, .34f, 1f, .72f, .48f };
        var result = new VertexPositionColor[indices.Length];
        for (var i = 0; i < result.Length; i++) result[i] = new VertexPositionColor(points[indices[i]], Shade(color, brightness[i / 6]));
        return result;
    }

    private static byte[] GlyphRows(char c) => c switch
    {
        'A' => new byte[]{14,17,17,31,17,17,17}, 'C' => new byte[]{14,17,16,16,16,17,14},
        'D' => new byte[]{30,17,17,17,17,17,30}, 'E' => new byte[]{31,16,16,30,16,16,31},
        'F' => new byte[]{31,16,16,30,16,16,16}, 'I' => new byte[]{14,4,4,4,4,4,14},
        'L' => new byte[]{16,16,16,16,16,16,31}, 'M' => new byte[]{17,27,21,21,17,17,17},
        'N' => new byte[]{17,25,21,19,17,17,17}, 'O' => new byte[]{14,17,17,17,17,17,14},
        'P' => new byte[]{30,17,17,30,16,16,16}, 'R' => new byte[]{30,17,17,30,20,18,17},
        'S' => new byte[]{15,16,16,14,1,1,30}, 'T' => new byte[]{31,4,4,4,4,4,4},
        'U' => new byte[]{17,17,17,17,17,17,14}, 'V' => new byte[]{17,17,17,17,17,10,4},
        'W' => new byte[]{17,17,17,21,21,21,10}, 'Y' => new byte[]{17,17,10,4,4,4,4},
        '0' => new byte[]{14,17,19,21,25,17,14}, '1' => new byte[]{4,12,4,4,4,4,14},
        '2' => new byte[]{14,17,1,2,4,8,31}, '3' => new byte[]{30,1,1,14,1,1,30},
        '/' => new byte[]{1,1,2,4,8,16,16}, _ => new byte[7]
    };

    private static (Color Sky, Color Floor, Color Grid, Color Boundary, Color Accent) ThemeForSector(int index) => index switch
    {
        1 => (new Color(13,5,27), new Color(55,30,85), new Color(130,65,150), new Color(30,220,220), new Color(205,80,255)),
        2 => (new Color(25,5,8), new Color(72,27,25), new Color(150,58,35), new Color(255,185,35), new Color(255,75,35)),
        _ => (new Color(4,10,30), new Color(25,45,90), new Color(55,90,135), new Color(255,120,20), new Color(0,175,225))
    };

    private static string SectorName(int index) => index switch { 1 => "ION BASIN", 2 => "SOLAR FORGE", _ => "STARPORT" };
    private static float DistanceSquaredXZ(Vector3 a, Vector3 b) { var x = a.X - b.X; var z = a.Z - b.Z; return x*x + z*z; }
    private static Color Shade(Color color, float amount) => new(
        Math.Clamp((int)MathF.Round(color.R * amount), 0, 255),
        Math.Clamp((int)MathF.Round(color.G * amount), 0, 255),
        Math.Clamp((int)MathF.Round(color.B * amount), 0, 255), color.A);
    private static Vector3 LocalOffset(Vector3 origin, float heading, float right, float up, float forward) =>
        new(origin.X + MathF.Cos(heading)*right + MathF.Sin(heading)*forward,
            origin.Y + up, origin.Z + MathF.Sin(heading)*right - MathF.Cos(heading)*forward);
    private static float CenteredX(int length, float pixelSize) => (ReferenceWidth - (length * 6f - 1f) * pixelSize) * 0.5f;
}
