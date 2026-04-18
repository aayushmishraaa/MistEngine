-- orbits.lua — spawns a ring of cubes, each with its own spinner script.
-- Loaded by bootstrap.lua at engine startup; demonstrates spawn_cube +
-- attach_script from a single Lua file.

local NUM_CUBES     = 5
local ORBIT_RADIUS  = 3.0
local ORBIT_HEIGHT  = 1.0

for i = 1, NUM_CUBES do
    local theta = ((i - 1) / NUM_CUBES) * (2 * math.pi)
    local x = math.cos(theta) * ORBIT_RADIUS
    local z = math.sin(theta) * ORBIT_RADIUS

    local e = spawn_cube(x, ORBIT_HEIGHT, z)
    if e >= 0 then
        attach_script(e, 'res://scripts/spinner.lua')
    end
end
