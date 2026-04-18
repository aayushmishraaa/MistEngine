-- bootstrap.lua — the engine's opening scene, in Lua. Replaces the
-- previously-hardcoded C++ setup that built ground + cube. Anything you
-- want in the default editor view goes here; no engine rebuild needed.

print("bootstrap: assembling default scene")

-- Ground plane, slightly below y=0 so nothing z-fights with it.
spawn_plane(0, -0.01, 0, 20, 1, 20)

-- Delegates to the orbit demo, which spawns the ring of cubes.
run_script('res://scripts/orbits.lua')
