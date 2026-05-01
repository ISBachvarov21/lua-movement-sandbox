function update(player, input, dt)
    local speed = 10000
    local jump_velocity = -1000

    if input.aPressed then
        player.vel.x = player.vel.x - speed * dt
    end

    if input.dPressed then
        player.vel.x = player.vel.x + speed * dt
    end

    if input.jump then
        print("jumping")
        player:apply_velocity(Vec2:new(0, jump_velocity))
    end

    if player.pos.x > 1280 then
        player:set_position(Vec2:new(-50, player.pos.y))
    elseif player.pos.x < -50 then
        player:set_position(Vec2:new(1280-50, player.pos.y))
    end
end