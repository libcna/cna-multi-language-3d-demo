#!/usr/bin/env ruby
# frozen_string_literal: true

class Game
  attr_reader :state, :mask, :x, :z, :elapsed, :score
  def initialize; reset; end
  def reset
    @state = 0; @mask = 0; @x = @z = @elapsed = @score = @hazard_x = 0.0; @hazard_direction = 1.0
  end
  def d2(ax, az, bx, bz); (ax - bx)**2 + (az - bz)**2; end
  def update(seconds, mx = 0, mz = 0, boost = false, restart = false)
    if restart then reset; return end
    return if [2, 3].include?(@state)
    @state = 1 if @state == 0; dt = [[seconds, 0].max, 0.25].min; speed = boost ? 7.0 : 4.0
    @x = [[@x + [[mx, -1].max, 1].min * speed * dt, -10].max, 10].min
    @z = [[@z + [[mz, -1].max, 1].min * speed * dt, -10].max, 10].min; @elapsed += dt
    @hazard_x += @hazard_direction * 2 * dt
    if @hazard_x >= 7 then @hazard_x = 7; @hazard_direction = -1 end
    if @hazard_x <= -7 then @hazard_x = -7; @hazard_direction = 1 end
    [[-6, 0], [0, -5], [6, 0]].each_with_index do |cell, i|
      bit = 1 << i; if (@mask & bit).zero? && d2(@x, @z, *cell) <= 0.81 then @mask |= bit; @score += 100 end
    end
    if d2(@x, @z, @hazard_x, 3) <= 3.0625 || @elapsed >= 60 then @state = 3
    elsif @mask == 7 && d2(@x, @z, 0, -9) <= 1.96 then @state = 2; @score += 1000 end
  end
end
def step(g, x, z, boost = true); g.update(0.25, x, z, boost); end
def collect(g); 4.times { step(g, -1, 0) }; 4.times { step(g, 1, 0) }; 3.times { step(g, 0, -1) }; 4.times { step(g, 1, 0) }; end
name = ARGV[0] || 'startup'; g = Game.new
case name
when 'collection', 'win'; collect(g); if name == 'win' then 3.times { step(g, -1, 0) }; 2.times { step(g, 0, -1) } end
when 'hazard'; 3.times { step(g, 0, 1, false) }
when 'loss'; 241.times { step(g, 0, 0, false) }
when 'restart'; step(g, 1, 0); g.update(0, 0, 0, false, true)
when 'startup'
else exit 2
end
puts format('%d %d %.4f %.4f %.4f %.4f', g.state, g.mask, g.x, g.z, g.elapsed, g.score)