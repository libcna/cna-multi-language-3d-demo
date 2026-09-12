#!/usr/bin/env python3
"""Small headless implementation of the shared CNA Starfield Courier contract."""
import sys


class Game:
    def __init__(self):
        self.reset()

    def reset(self):
        self.state, self.mask = 0, 0
        self.x = self.z = self.elapsed = self.score = self.hazard_x = 0.0
        self.hazard_direction = 1.0

    @staticmethod
    def distance2(ax, az, bx, bz):
        return (ax - bx) ** 2 + (az - bz) ** 2

    def update(self, seconds, move_x=0.0, move_z=0.0, boost=False, restart=False):
        if restart:
            self.reset()
            return
        if self.state in (2, 3):
            return
        if self.state == 0:
            self.state = 1
        dt = max(0.0, min(0.25, seconds))
        speed = 7.0 if boost else 4.0
        self.x = max(-10.0, min(10.0, self.x + max(-1.0, min(1.0, move_x)) * speed * dt))
        self.z = max(-10.0, min(10.0, self.z + max(-1.0, min(1.0, move_z)) * speed * dt))
        self.elapsed += dt
        self.hazard_x += self.hazard_direction * 2.0 * dt
        if self.hazard_x >= 7:
            self.hazard_x, self.hazard_direction = 7.0, -1.0
        elif self.hazard_x <= -7:
            self.hazard_x, self.hazard_direction = -7.0, 1.0
        for index, cell in enumerate(((-6, 0), (0, -5), (6, 0))):
            if not self.mask & (1 << index) and self.distance2(self.x, self.z, *cell) <= 0.81:
                self.mask |= 1 << index
                self.score += 100.0
        if self.distance2(self.x, self.z, self.hazard_x, 3) <= 3.0625 or self.elapsed >= 60:
            self.state = 3
        elif self.mask == 7 and self.distance2(self.x, self.z, 0, -9) <= 1.96:
            self.state, self.score = 2, self.score + 1000

    def snapshot(self):
        return self.state, self.mask, self.x, self.z, self.elapsed, self.score


def step(game, x, z, boost=True):
    game.update(0.25, x, z, boost)


def collect(game):
    for _ in range(4): step(game, -1, 0)
    for _ in range(4): step(game, 1, 0)
    for _ in range(3): step(game, 0, -1)
    for _ in range(4): step(game, 1, 0)


def scenario(name):
    game = Game()
    if name in ("collection", "win"):
        collect(game)
        if name == "win":
            for _ in range(3): step(game, -1, 0)
            for _ in range(2): step(game, 0, -1)
    elif name == "hazard":
        for _ in range(3): step(game, 0, 1, False)
    elif name == "loss":
        for _ in range(241): step(game, 0, 0, False)
    elif name == "restart":
        step(game, 1, 0)
        game.update(0, restart=True)
    elif name != "startup":
        raise SystemExit(f"unknown scenario: {name}")
    return game


if __name__ == "__main__":
    values = scenario(sys.argv[1] if len(sys.argv) > 1 else "startup").snapshot()
    print(values[0], values[1], *(f"{v:.4f}" for v in values[2:]))