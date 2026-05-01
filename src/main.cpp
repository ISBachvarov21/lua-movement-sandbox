#include <sol/sol.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>

struct Input {
	bool aPressed = false;
	bool dPressed = false;
	bool jump = false;
};

struct Player {
	sf::Vector2f pos{100, 0};
	sf::Vector2f vel{0,0};
	bool onGround = false;
	sf::RectangleShape shape;

	void apply_velocity(const sf::Vector2f v) {
		vel.x += v.x;
		vel.y += v.y;
	}
	void set_velocity(const sf::Vector2f v) {
		vel = v;
	}
	void move(const sf::Vector2f v) {
		pos += v;
		shape.move(v);
	}
	void setPosition(const sf::Vector2f v) {
		pos = v;
		shape.setPosition(v);
	}
};

int main() {
	// sol::state lua;
	// lua.open_libraries(sol::lib::base);
	//
	// lua.script("print('Lua works')");

	sf::RenderWindow window(sf::VideoMode({1280, 720}), "SFML works");

	bool running = true;

	Player player;
	player.shape = sf::RectangleShape({50, 100});
	player.shape.setPosition({100, 100});
	player.shape.setFillColor(sf::Color::Red);

	Input input;

	sf::Clock clock;
	float dt = clock.restart().asSeconds();
	bool isJumping = false;

	while (running) {
		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				running = false;
				window.close();
			}
		}

		input = {false, false, input.jump};
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
			input.aPressed = true;
			player.vel.x -= 60;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
			input.dPressed = true;
			player.vel.x += 60;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && !input.jump) {
			input.jump = true;
			player.vel.y = -980;
		}

		// physics
		player.vel.y += 3920 * dt;
		if (!input.aPressed && !input.dPressed) {
			player.vel.x *= 0.1;
		}

		// clamp velocity
		player.vel.x = std::clamp(player.vel.x, -600.f, 600.f);
		player.vel.y = std::clamp(player.vel.y, -3920.f, 3920.f);

		player.move(player.vel * dt);

		if (player.pos.y > 620.f) {
			player.setPosition({player.pos.x, 620.f});
			player.vel.y = 0;
			input.jump = false;
		}

		window.clear();
		window.draw(player.shape);
		window.display();

		dt = clock.restart().asSeconds();
	}

	return 0;
}
