#include <sol/sol.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>

int main() {
	// sol::state lua;
	// lua.open_libraries(sol::lib::base);
	//
	// lua.script("print('Lua works')");

	sf::RenderWindow window(sf::VideoMode({1280, 720}), "SFML works");

	bool running = true;

	sf::RectangleShape player({50, 100});
	player.setPosition({100, 100});
	player.setFillColor(sf::Color::Red);
	sf::Vector2f playerVelocity = {0, 0};

	sf::Clock clock;
	float dt = clock.restart().asSeconds();
	bool isJumping = false;
	bool isMoving = false;

	while (running) {
		isMoving = false;

		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				running = false;
				window.close();
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
			isMoving = true;
			playerVelocity.x -= 60;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
			isMoving = true;
			playerVelocity.x += 60;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && !isJumping) {
			isJumping = true;
			playerVelocity.y = -980;
		}

		// physics
		playerVelocity.y += 3920 * dt;
		if (!isMoving) {
			playerVelocity.x *= 0.1;
		}

		// clamp velocity
		playerVelocity.x = std::clamp(playerVelocity.x, -600.f, 600.f);
		playerVelocity.y = std::clamp(playerVelocity.y, -3920.f, 3920.f);

		player.move(playerVelocity * dt);

		sf::Vector2f pos = player.getPosition();
		if (pos.y > 620.f) {
			player.setPosition({pos.x, 620.f});
			playerVelocity.y = 0;
			isJumping = false;
		}

		window.clear();
		window.draw(player);
		window.display();

		dt = clock.restart().asSeconds();
	}

	return 0;
}
