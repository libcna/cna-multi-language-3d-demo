#[derive(Clone, Copy, PartialEq)]
enum State { Title, Playing, Won, Lost }

struct Game { state: State, mask: u32, x: f32, z: f32, elapsed: f32, score: f32, hazard_x: f32, hazard_direction: f32 }
impl Game {
    fn new() -> Self { Self { state: State::Title, mask: 0, x: 0.0, z: 0.0, elapsed: 0.0, score: 0.0, hazard_x: 0.0, hazard_direction: 1.0 } }
    fn reset(&mut self) { *self = Self::new(); }
    fn d2(ax: f32, az: f32, bx: f32, bz: f32) -> f32 { (ax-bx).powi(2) + (az-bz).powi(2) }
    fn update(&mut self, seconds: f32, mx: f32, mz: f32, boost: bool, restart: bool) {
        if restart { self.reset(); return; }
        if self.state == State::Won || self.state == State::Lost { return; }
        if self.state == State::Title { self.state = State::Playing; }
        let dt = seconds.max(0.0).min(0.25); let speed = if boost { 7.0 } else { 4.0 };
        self.x = (self.x + mx.clamp(-1.0, 1.0) * speed * dt).clamp(-10.0, 10.0);
        self.z = (self.z + mz.clamp(-1.0, 1.0) * speed * dt).clamp(-10.0, 10.0);
        self.elapsed += dt; self.hazard_x += self.hazard_direction * 2.0 * dt;
        if self.hazard_x >= 7.0 { self.hazard_x = 7.0; self.hazard_direction = -1.0; }
        if self.hazard_x <= -7.0 { self.hazard_x = -7.0; self.hazard_direction = 1.0; }
        for (i, (cx, cz)) in [(-6.0, 0.0), (0.0, -5.0), (6.0, 0.0)].iter().enumerate() {
            if self.mask & (1 << i) == 0 && Self::d2(self.x, self.z, *cx, *cz) <= 0.81 { self.mask |= 1 << i; self.score += 100.0; }
        }
        if Self::d2(self.x, self.z, self.hazard_x, 3.0) <= 3.0625 || self.elapsed >= 60.0 { self.state = State::Lost; }
        else if self.mask == 7 && Self::d2(self.x, self.z, 0.0, -9.0) <= 1.96 { self.state = State::Won; self.score += 1000.0; }
    }
}
fn step(g: &mut Game, x: f32, z: f32, boost: bool) { g.update(0.25, x, z, boost, false); }
fn collect(g: &mut Game) { for _ in 0..4 { step(g, -1.0, 0.0, true); } for _ in 0..4 { step(g, 1.0, 0.0, true); } for _ in 0..3 { step(g, 0.0, -1.0, true); } for _ in 0..4 { step(g, 1.0, 0.0, true); } }
fn main() {
    let name = std::env::args().nth(1).unwrap_or_else(|| "startup".into()); let mut g = Game::new();
    match name.as_str() {
        "startup" => (), "collection" | "win" => { collect(&mut g); if name == "win" { for _ in 0..3 { step(&mut g, -1.0, 0.0, true); } for _ in 0..2 { step(&mut g, 0.0, -1.0, true); } } },
        "hazard" => for _ in 0..3 { step(&mut g, 0.0, 1.0, false); }, "loss" => for _ in 0..241 { step(&mut g, 0.0, 0.0, false); },
        "restart" => { step(&mut g, 1.0, 0.0, true); g.update(0.0, 0.0, 0.0, false, true); }, _ => std::process::exit(2),
    }
    let state = match g.state { State::Title => 0, State::Playing => 1, State::Won => 2, State::Lost => 3 };
    println!("{} {} {:.4} {:.4} {:.4} {:.4}", state, g.mask, g.x, g.z, g.elapsed, g.score);
}