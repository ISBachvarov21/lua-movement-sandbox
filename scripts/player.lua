function update(player, input, dt)
    if input.aPressed then
        player.vel.x = player.vel.x - 600 * dt
    end

    if input.dPressed then
        player.vel.x = player.vel.x + 600 * dt
    end

    if input.jump and player.onGround then
        player.vel.y = -980
    end
end