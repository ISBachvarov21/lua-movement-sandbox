function update(player, input, dt)
    local speed = 4000
    local jump_velocity = -1000
    local max_jump_velocity = -1500

    if input.a_pressed then
        local vel = player.vel
        vel.x = vel.x - speed * dt
        player.vel = vel
    end

    if input.d_pressed then
        local vel = player.vel
        vel.x = vel.x + speed * dt
        player.vel = vel
    end

    -- FIX: jump count is at 0 after jumping
    if ((input.jump and (player.on_ground or player.coyote_jump_available)) and player.jump_count < 1)
        or (input.jump and player.jump_count < 1) then
        print(player.jump_count)

        -- subtract player velocity on y axis from jump velocity so that spamming double jump and timing double jump
        -- result in jumping the same height
        -- also works for ignoring downwards velocity so that the player can jump up instead of slowing their falling
        -- with their jumps
        if not player.on_ground then
            jump_velocity = jump_velocity - player.vel.y
        end
        player:apply_velocity(Vec2:new(0, jump_velocity))
        player.jump_count = player.jump_count + 1
    end

    local pos = player.pos
    if pos.x > 1280 then
        player:set_position(Vec2:new(-50, pos.y))
    elseif pos.x < -50 then
        player:set_position(Vec2:new(1280-50, pos.y))
    end

    if pos.y > 720 then
        player:set_position(Vec2:new(pos.x, -100))
    end
end