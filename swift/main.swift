import Foundation

enum State: Int { case title, playing, won, lost }
struct Game {
    var state = State.title, mask = 0, x = Float(0), z = Float(0), elapsed = Float(0), score = Float(0), hazardX = Float(0), hazardDirection = Float(1)
    mutating func reset() { self = Game() }
    static func d2(_ ax: Float, _ az: Float, _ bx: Float, _ bz: Float) -> Float { (ax-bx)*(ax-bx) + (az-bz)*(az-bz) }
    mutating func update(_ seconds: Float, _ mx: Float = 0, _ mz: Float = 0, _ boost: Bool = false, _ restart: Bool = false) {
        if restart { reset(); return }; if state == .won || state == .lost { return }; if state == .title { state = .playing }
        let dt = max(0, min(0.25, seconds)), speed: Float = boost ? 7 : 4
        x = max(-10, min(10, x + max(-1, min(1, mx))*speed*dt)); z = max(-10, min(10, z + max(-1, min(1, mz))*speed*dt)); elapsed += dt
        hazardX += hazardDirection*2*dt; if hazardX >= 7 { hazardX = 7; hazardDirection = -1 }; if hazardX <= -7 { hazardX = -7; hazardDirection = 1 }
        let cells: [(Float, Float)] = [(-6,0), (0,-5), (6,0)]; for (i, c) in cells.enumerated() { if mask & (1 << i) == 0 && Game.d2(x,z,c.0,c.1) <= 0.81 { mask |= 1 << i; score += 100 } }
        if Game.d2(x,z,hazardX,3) <= 3.0625 || elapsed >= 60 { state = .lost } else if mask == 7 && Game.d2(x,z,0,-9) <= 1.96 { state = .won; score += 1000 }
    }
}
func step(_ g: inout Game, _ x: Float, _ z: Float, _ boost: Bool = true) { g.update(0.25,x,z,boost) }
func collect(_ g: inout Game) { for _ in 0..<4 { step(&g,-1,0) }; for _ in 0..<4 { step(&g,1,0) }; for _ in 0..<3 { step(&g,0,-1) }; for _ in 0..<4 { step(&g,1,0) } }
let name = CommandLine.arguments.dropFirst().first ?? "startup"; var g = Game()
switch name { case "startup": break; case "collection", "win": collect(&g); if name == "win" { for _ in 0..<3 { step(&g,-1,0) }; for _ in 0..<2 { step(&g,0,-1) } }; case "hazard": for _ in 0..<3 { step(&g,0,1,false) }; case "loss": for _ in 0..<241 { step(&g,0,0,false) }; case "restart": step(&g,1,0); g.update(0,0,0,false,true); default: exit(2) }
print("\(g.state.rawValue) \(g.mask) \(String(format: \"%.4f\", g.x)) \(String(format: \"%.4f\", g.z)) \(String(format: \"%.4f\", g.elapsed)) \(String(format: \"%.4f\", g.score))")