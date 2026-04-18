-- spinner.lua — demo gameplay script. Attaches via ScriptComponent; the
-- engine calls _ready() once during HierarchySystem's post-order ready
-- walk, then _process() every frame. Edit `spin_speed` below to change
-- how fast the attached entity rotates — the engine picks up the new
-- value on next launch.

local spin_speed = 45.0  -- degrees / second

function _ready()
    print("spinner attached to entity " .. entity_id())
end

function _process()
    local t = get_transform()
    if not t then return end

    local dt = get_delta_time()
    t.ry = t.ry + spin_speed * dt
    set_transform(t)
end
