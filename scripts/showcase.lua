-- showcase.lua — feature demo scene.
--
-- Shows off everything shipped across the recent cycles:
--
--   * PBR + CSM shadows         (ground plane + tall pillars)
--   * TAA / FXAA / motion blur  (spinning cubes at varied speeds)
--   * SSR                       (ground reflects the whole scene)
--   * DOF                       (cubes receding into Z)
--   * Shortcuts / gizmos        (select any cube, press W/E/R)
--   * Inspector + undo          (drag gizmo, Ctrl+Z collapses to one step)
--   * Hierarchy drag-drop       (drag cubes onto each other)
--   * Theme / AgX tonemap       (View menu submenus)
--
-- LAYOUT (camera sits at (0, 5, 14) looking -Z, pitched down 18°):
--
--    near row  z=-2   PILLARS (1 x 5 x 1)    <- tall, foregrounded
--    mid row   z=-6   spinners (1 x 1 x 1)   <- attach spinner.lua
--    far row   z=-10..-22  small cubes       <- depth spread for DOF
--    hero cube (0, 2, -4) spinner            <- central demo target
--    ground plane y=0, 30x30                 <- reflector + shadow catcher

print("showcase: assembling feature-demo scene")

-- One ground plane. 30x30 so shadow/reflection footprint exceeds the
-- scene bounds — keeps SSR from running off the edge mid-pillar.
spawn_plane(0, 0, 0, 30.0, 1.0, 30.0)

-- Near row: 5 tall pillars (scale 1 x 5 x 1, planted at y=2.5 so
-- their base sits on the ground). Long shadows, clear foreground.
for i = 1, 5 do
    local x = -8.0 + (i - 1) * 4.0
    spawn_cube(x, 2.5, -2.0, 1.0, 5.0, 1.0)
end

-- Mid row: 5 unit cubes with spinner.lua attached. Rotate on Y for
-- motion blur visibility. Slightly off-center X so they don't hide
-- behind the pillars from the default camera angle.
for i = 1, 5 do
    local x = -7.0 + (i - 1) * 3.5
    local e = spawn_cube(x, 1.0, -6.0)
    if e >= 0 then
        attach_script(e, 'res://scripts/spinner.lua')
    end
end

-- Far row: smaller cubes (0.7 scale) receding to z=-22. DOF with
-- focus ~6 blurs them; they also give SSR something to reflect in
-- the back of the ground plane.
for i = 1, 5 do
    local x = -6.0 + (i - 1) * 3.0
    local z = -10.0 - (i - 1) * 3.0
    spawn_cube(x, 0.7, z, 0.7, 0.7, 0.7)
end

-- Hero: central floating spinner at eye level. Select from the
-- Hierarchy panel to exercise gizmos / inspector / undo.
local hero = spawn_cube(0.0, 2.0, -4.0, 1.5, 1.5, 1.5)
if hero >= 0 then
    attach_script(hero, 'res://scripts/spinner.lua')
end

-- Directional light (the "sun"). Phase F couples this to the sky —
-- rotate the entity in the Inspector and the sky's sun direction
-- follows. Energy tuned so sunlit surfaces hit a pleasant mid-tone
-- under AgX tonemap.
local sun = spawn_light("directional", 0.0, 10.0, 0.0, 1.0, 0.97, 0.9, 2.5)

-- Point lights — three coloured omni lights floating between the
-- pillars. Tests Phase B (clustered shader loop) + Phase C (ECS
-- LightComponent). Pick bright-ish energy values so the
-- contribution is visible without overpowering the sun.
spawn_light("omni", -6.0, 2.0, -4.0, 1.0, 0.2, 0.2, 10.0)  -- red
spawn_light("omni",  0.0, 3.5, -6.0, 0.2, 1.0, 0.3, 10.0)  -- green
spawn_light("omni",  6.0, 2.0, -4.0, 0.2, 0.4, 1.0, 10.0)  -- blue

print("showcase: 16 cubes + 1 ground plane + 3 colored point lights ready")
print("  WASD/QE to fly; right-click to mouse look; Numpad 0 resets view")
print("  View -> Post-Processing  -> toggle SSR / Motion Blur / DOF")
print("  View -> Tonemap          -> ACES / Reinhard / AgX (default)")
print("  Select a cube then press W/E/R for translate/rotate/scale")
