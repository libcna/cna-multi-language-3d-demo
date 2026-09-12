declare const process: { argv: string[]; exit(code: number): never };

enum State { Title, Playing, Won, Lost }

class Game {
  state = State.Title; mask = 0; x = 0; z = 0; elapsed = 0; score = 0;
  hazardX = 0; hazardDirection = 1;
  reset(): void { this.state = State.Title; this.mask = 0; this.x = this.z = this.elapsed = this.score = this.hazardX = 0; this.hazardDirection = 1; }
  static d2(ax: number, az: number, bx: number, bz: number): number { return (ax - bx) ** 2 + (az - bz) ** 2; }
  update(seconds: number, mx = 0, mz = 0, boost = false, restart = false): void {
    if (restart) { this.reset(); return; }
    if (this.state === State.Won || this.state === State.Lost) return;
    if (this.state === State.Title) this.state = State.Playing;
    const dt = Math.max(0, Math.min(.25, seconds)); const speed = boost ? 7 : 4;
    this.x = Math.max(-10, Math.min(10, this.x + Math.max(-1, Math.min(1, mx)) * speed * dt));
    this.z = Math.max(-10, Math.min(10, this.z + Math.max(-1, Math.min(1, mz)) * speed * dt));
    this.elapsed += dt; this.hazardX += this.hazardDirection * 2 * dt;
    if (this.hazardX >= 7) { this.hazardX = 7; this.hazardDirection = -1; }
    if (this.hazardX <= -7) { this.hazardX = -7; this.hazardDirection = 1; }
    const cells = [[-6, 0], [0, -5], [6, 0]];
    cells.forEach(([cx, cz], i) => { if (!(this.mask & (1 << i)) && Game.d2(this.x, this.z, cx, cz) <= .81) { this.mask |= 1 << i; this.score += 100; } });
    if (Game.d2(this.x, this.z, this.hazardX, 3) <= 3.0625 || this.elapsed >= 60) this.state = State.Lost;
    else if (this.mask === 7 && Game.d2(this.x, this.z, 0, -9) <= 1.96) { this.state = State.Won; this.score += 1000; }
  }
}
function step(g: Game, x: number, z: number, boost = true): void { g.update(.25, x, z, boost); }
function collect(g: Game): void { for (let i = 0; i < 4; i++) step(g, -1, 0); for (let i = 0; i < 4; i++) step(g, 1, 0); for (let i = 0; i < 3; i++) step(g, 0, -1); for (let i = 0; i < 4; i++) step(g, 1, 0); }
const name = process.argv[2] || "startup"; const g = new Game();
if (name === "collection" || name === "win") { collect(g); if (name === "win") { for (let i = 0; i < 3; i++) step(g, -1, 0); for (let i = 0; i < 2; i++) step(g, 0, -1); } }
else if (name === "hazard") for (let i = 0; i < 3; i++) step(g, 0, 1, false);
else if (name === "loss") for (let i = 0; i < 241; i++) step(g, 0, 0, false);
else if (name === "restart") { step(g, 1, 0); g.update(0, 0, 0, false, true); }
else if (name !== "startup") process.exit(2);
const s = g.state; console.log(`${s} ${g.mask} ${g.x.toFixed(4)} ${g.z.toFixed(4)} ${g.elapsed.toFixed(4)} ${g.score.toFixed(4)}`);