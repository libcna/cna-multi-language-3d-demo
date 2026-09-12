package main

import (
	"fmt"
	"math"
	"os"
)

type State int
const ( Title State = iota; Playing; Won; Lost )
type Game struct { state State; mask uint32; x, z, elapsed, score, hazardX, hazardDirection float64 }
func NewGame() *Game { return &Game{hazardDirection: 1} }
func (g *Game) Reset() { *g = Game{hazardDirection: 1} }
func d2(ax, az, bx, bz float64) float64 { return (ax-bx)*(ax-bx) + (az-bz)*(az-bz) }
func clamp(v, low, high float64) float64 { return math.Max(low, math.Min(high, v)) }
func (g *Game) Update(seconds, mx, mz float64, boost, restart bool) {
	if restart { g.Reset(); return }; if g.state == Won || g.state == Lost { return }; if g.state == Title { g.state = Playing }
	dt := clamp(seconds, 0, .25); speed := 4.0; if boost { speed = 7 }
	g.x = clamp(g.x+clamp(mx, -1, 1)*speed*dt, -10, 10); g.z = clamp(g.z+clamp(mz, -1, 1)*speed*dt, -10, 10)
	g.elapsed += dt; g.hazardX += g.hazardDirection*2*dt
	if g.hazardX >= 7 { g.hazardX = 7; g.hazardDirection = -1 }; if g.hazardX <= -7 { g.hazardX = -7; g.hazardDirection = 1 }
	cells := [][2]float64{{-6, 0}, {0, -5}, {6, 0}}
	for i, c := range cells { bit := uint32(1 << i); if g.mask&bit == 0 && d2(g.x, g.z, c[0], c[1]) <= .81 { g.mask |= bit; g.score += 100 } }
	if d2(g.x, g.z, g.hazardX, 3) <= 3.0625 || g.elapsed >= 60 { g.state = Lost; return }; if g.mask == 7 && d2(g.x, g.z, 0, -9) <= 1.96 { g.state = Won; g.score += 1000 }
}
func step(g *Game, x, z float64, boost bool) { g.Update(.25, x, z, boost, false) }
func collect(g *Game) { for i:=0; i<4; i++ { step(g,-1,0,true) }; for i:=0; i<4; i++ { step(g,1,0,true) }; for i:=0; i<3; i++ { step(g,0,-1,true) }; for i:=0; i<4; i++ { step(g,1,0,true) } }
func main() {
	g := NewGame(); name := "startup"; if len(os.Args)>1 { name = os.Args[1] }; switch name {
	case "startup":
	case "collection", "win": collect(g); if name == "win" { for i:=0; i<3; i++ { step(g,-1,0,true) }; for i:=0; i<2; i++ { step(g,0,-1,true) } }
	case "hazard": for i:=0; i<3; i++ { step(g,0,1,false) }
	case "loss": for i:=0; i<241; i++ { step(g,0,0,false) }
	case "restart": step(g,1,0,true); g.Update(0,0,0,false,true)
	default: os.Exit(2)
	}; fmt.Printf("%d %d %.4f %.4f %.4f %.4f\n", g.state, g.mask, g.x, g.z, g.elapsed, g.score)
}