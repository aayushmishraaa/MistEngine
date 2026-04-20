-- bootstrap.lua — the engine's opening scene. Delegates to the
-- showcase scene (rich demo covering every post-effect + editor
-- feature). To go back to the simpler ring demo, swap `showcase.lua`
-- for `orbits.lua`.
--
-- Ground plane, cubes, and spinners are all spawned inside the
-- delegated script — bootstrap itself adds nothing so there are no
-- overlapping entities to z-fight.

print("bootstrap: assembling default scene")

run_script('res://scripts/showcase.lua')
