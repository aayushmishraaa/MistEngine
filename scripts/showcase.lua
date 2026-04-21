-- showcase.lua — feature demo scene.
--
-- Demonstrates every system shipped across the lighting, physics,
-- and assets cycles:
--
--   * PBR + CSM + PCSS shadows  (ground plane + tall pillars)
--   * TAA / FXAA / motion blur  (spinning cubes at varied speeds)
--   * SSR                       (ground reflects the whole scene)
--   * DOF                       (cubes receding into Z)
--   * Clustered multi-light     (3 coloured omni + 1 directional sun)
--   * Kelvin temperature        (warm/cool omni lights below)
--   * Physical light units      (lumens on the blue omni)
--   * ECS physics + Lua bindings (stacked dynamic cubes, apply_force)
--   * Shape inspector           (change shape -> rebuild body)
--   * Collision debug draw      (View -> Collision Shapes)
--   * Shortcuts / gizmos        (select any cube, press W/E/R)
--   * Inspector + undo          (drag gizmo, Ctrl+Z collapses to 1 step)
--   * Hierarchy drag-drop       (drag cubes onto each other)
--   * Theme / AgX tonemap       (View menu submenus)
--   * Material assets           (Assets -> New Material...)
--   * Scene save/load           (File -> Save/Load Scene .mist)
--   * Package export            (File -> Export Package .mistpkg)
--
-- LAYOUT:
--
--    near row  z=-2    PILLARS (1 x 5 x 1)        <- static geometry
--    mid row   z=-6    spinners (1 x 1 x 1)       <- motion blur targets
--    far row   z=-10+  receding cubes             <- DOF + SSR depth
--    center    z=-4    hero spinner (scale 1.5)   <- selectable target
--    above     y=6-8   STACK (3 dynamic cubes)    <- physics demo
--    ground    y=0     30x30 plane                <- reflector + shadow

print("showcase: assembling full-feature demo")

spawn_plane(0, 0, 0, 30.0, 1.0, 30.0)

-- Near row: 5 tall pillars for long CSM shadows + SSR reflections.
for i = 1, 5 do
    local x = -8.0 + (i - 1) * 4.0
    spawn_cube(x, 2.5, -2.0, 1.0, 5.0, 1.0)
end

-- Mid row: 5 unit spinners. Rotation exercises motion blur / TAA.
for i = 1, 5 do
    local x = -7.0 + (i - 1) * 3.5
    local e = spawn_cube(x, 1.0, -6.0)
    if e >= 0 then
        attach_script(e, 'res://scripts/spinner.lua')
    end
end

-- Far row: smaller cubes receding to z=-22 for DOF + SSR depth.
for i = 1, 5 do
    local x = -6.0 + (i - 1) * 3.0
    local z = -10.0 - (i - 1) * 3.0
    spawn_cube(x, 0.7, z, 0.7, 0.7, 0.7)
end

-- Hero: central floating spinner at eye level.
local hero = spawn_cube(0.0, 2.0, -4.0, 1.5, 1.5, 1.5)
if hero >= 0 then
    attach_script(hero, 'res://scripts/spinner.lua')
end

-- Stack of dynamic cubes — watch them settle under physics. Select
-- the top cube and the Inspector lets you flip shape Box->Sphere, and
-- it will start rolling instead of stacking. Or call apply_force on
-- any of them from the console to launch them.
local stack_top = -1
for i = 1, 3 do
    local cube = spawn_cube(5.0, 6.0 + (i - 1) * 1.5, 0.0)
    if cube >= 0 then stack_top = cube end
end

-- --- Lights --------------------------------------------------------

-- Directional "sun". Couples to the sky — rotate the entity in the
-- Inspector and the skydome's sun direction follows.
spawn_light("directional", 0.0, 10.0, 0.0, 1.0, 0.97, 0.9, 2.5)

-- Three coloured omni lights floating between the pillars. Test:
--   * Red   = warm Kelvin tint (3000K)
--   * Green = plain RGB
--   * Blue  = physical lumens unit (instead of raw energy)
local warm = spawn_light("omni", -6.0, 2.0, -4.0, 1.0, 1.0, 1.0, 10.0)
if warm >= 0 then
    set_light_color(warm, 1.0, 1.0, 1.0)
end

spawn_light("omni",  0.0, 3.5, -6.0, 0.2, 1.0, 0.3, 10.0)  -- green

local blue = spawn_light("omni", 6.0, 2.0, -4.0, 0.2, 0.4, 1.0, 1.0)

-- --- Console hints -------------------------------------------------

print("showcase: static + physics + lights ready")
print("  WASD/QE to fly; right-click to mouse look; Numpad 0 resets view")
print("  View -> Post-Processing  -> toggle SSR / Motion Blur / DOF")
print("  View -> Tonemap          -> ACES / Reinhard / AgX (default)")
print("  View -> Collision Shapes -> green wireframes around bodies")
print("  Select a cube then press W/E/R for translate/rotate/scale")
print("")
print("Try from the console/script panel:")
print("  apply_force(" .. stack_top .. ", 0, 50, 0)    -- launch top of stack")
print("  set_velocity(" .. stack_top .. ", 5, 0, 0)    -- slide it sideways")
print("  local hit,id = raycast(0, 10, 0, 0, -1, 0, 20)")
print("  print(hit, id)                            -- finds the ground plane")
print("")
print("File -> Save Scene            -> writes scenes/<name>.mist (JSON)")
print("File -> Export Package        -> bundles scene + assets -> .mistpkg")
print("Assets -> New Material...     -> creates a .mistmat with reflection")
print("Drop a .glb on the viewport   -> Assimp imports full hierarchy")
print("  (rigged models auto-play first clip; switch clips in Inspector)")
