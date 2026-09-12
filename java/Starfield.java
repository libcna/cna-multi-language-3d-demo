import java.util.Locale;

public final class Starfield {
    enum State { TITLE, PLAYING, WON, LOST }
    static final class Game {
        State state = State.TITLE; int mask; float x, z, elapsed, score, hazardX, hazardDirection = 1;
        void reset() { state = State.TITLE; mask = 0; x = z = elapsed = score = hazardX = 0; hazardDirection = 1; }
        void update(float seconds, float mx, float mz, boolean boost, boolean restart) {
            if (restart) { reset(); return; }
            if (state == State.WON || state == State.LOST) return;
            if (state == State.TITLE) state = State.PLAYING;
            float dt = Math.max(0, Math.min(.25f, seconds));
            float speed = boost ? 7 : 4;
            x = clamp(x + clamp(mx) * speed * dt, -10, 10); z = clamp(z + clamp(mz) * speed * dt, -10, 10);
            elapsed += dt; hazardX += hazardDirection * 2 * dt;
            if (hazardX >= 7) { hazardX = 7; hazardDirection = -1; }
            if (hazardX <= -7) { hazardX = -7; hazardDirection = 1; }
            float[] cells = {-6, 0, 0, -5, 6, 0};
            for (int i = 0; i < 3; i++) if ((mask & (1 << i)) == 0 && d2(x, z, cells[i * 2], cells[i * 2 + 1]) <= .81f) { mask |= 1 << i; score += 100; }
            if (d2(x, z, hazardX, 3) <= 3.0625f || elapsed >= 60) { state = State.LOST; return; }
            if (mask == 7 && d2(x, z, 0, -9) <= 1.96f) { state = State.WON; score += 1000; }
        }
        static float clamp(float v) { return Math.max(-1, Math.min(1, v)); }
        static float clamp(float v, float low, float high) { return Math.max(low, Math.min(high, v)); }
        static float d2(float ax, float az, float bx, float bz) { float dx = ax - bx, dz = az - bz; return dx * dx + dz * dz; }
    }
    static void step(Game g, float x, float z) { step(g, x, z, true); }
    static void step(Game g, float x, float z, boolean boost) { g.update(.25f, x, z, boost, false); }
    static void collect(Game g) { for (int i = 0; i < 4; i++) step(g, -1, 0); for (int i = 0; i < 4; i++) step(g, 1, 0); for (int i = 0; i < 3; i++) step(g, 0, -1); for (int i = 0; i < 4; i++) step(g, 1, 0); }
    static void print(Game g) { System.out.printf(Locale.ROOT, "%d %d %.4f %.4f %.4f %.4f%n", g.state.ordinal(), g.mask, g.x, g.z, g.elapsed, g.score); }
    public static void main(String[] args) {
        Game g = new Game(); String scenario = args.length == 0 ? "startup" : args[0];
        if (scenario.equals("collection") || scenario.equals("win")) { collect(g); if (scenario.equals("win")) { for (int i = 0; i < 3; i++) step(g, -1, 0); for (int i = 0; i < 2; i++) step(g, 0, -1); } }
        else if (scenario.equals("hazard")) { step(g, 0, 1, false); step(g, 0, 1, false); step(g, 0, 1, false); }
        else if (scenario.equals("loss")) for (int i = 0; i < 241; i++) step(g, 0, 0, false);
        else if (scenario.equals("restart")) { step(g, 1, 0); g.update(0, 0, 0, false, true); }
        else if (!scenario.equals("startup")) { System.exit(2); }
        print(g);
    }
}