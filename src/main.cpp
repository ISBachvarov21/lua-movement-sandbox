#include <sol/sol.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

fs::file_time_type last_write_time;
sol::function update_fn;
sol::state lua;

void load_script() {
	lua.safe_script_file("../../scripts/player.lua");
	update_fn = lua["update"];
}

void try_reload() {
	auto current = fs::last_write_time("../../scripts/player.lua");

	if (current != last_write_time) {
		last_write_time = current;

		try {
			load_script();
			std::cout << "Lua reloaded\n";
		} catch (const std::exception& e) {
			std::cout << "Lua error: " << e.what() << "\n";
		}
	}
}

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

	void applyVelocity(const sf::Vector2f v) {
		vel += v;
	}
	void setVelocity(const sf::Vector2f v) {
		vel = v;
	}
	void move(const sf::Vector2f v) {
		pos += v;
		shape.move(v);
	}
	void setPosition(const sf::Vector2f v) {
		pos = v;
		shape.setPosition(pos);
	}
};

int main() {
	std::cout << "Opening libraries" << std::endl;
	lua.open_libraries(sol::lib::base, sol::lib::math);

	std::cout << "Exposing usertypes" << std::endl;
	lua.new_usertype<sf::Vector2f>("Vec2",
		sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>(),
		"x", &sf::Vector2f::x,
		"y", &sf::Vector2f::y
	);
	lua.new_usertype<Input>("Input",
		"aPressed", &Input::aPressed,
		"dPressed", &Input::dPressed,
		"jump", &Input::jump
	);


	sf::RenderWindow window(sf::VideoMode({1280, 720}), "SFML works");

	bool running = true;

	Player player;
	player.shape = sf::RectangleShape({50, 100});
	player.shape.setPosition({100, 100});
	player.shape.setFillColor(sf::Color::Red);

	lua.new_usertype<Player>("Player",
		"pos", &Player::pos,
		"vel", &Player::vel,
		"onGround", &Player::onGround,
		"apply_velocity", &Player::applyVelocity,
		"set_velocity", &Player::setVelocity,
		"move", &Player::move,
		"set_position", &Player::setPosition
	);

	Input input;

	sf::Clock clock;
	float dt = clock.restart().asSeconds();
	float timeSinceLastReload = 0;

	std::cout << "Entering game loop" << std::endl;

	window.setFramerateLimit(60);
	while (running) {
		if (timeSinceLastReload > 1) {
			try_reload();
			timeSinceLastReload = 0;
		}

		while (const std::optional event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				running = false;
				window.close();
			}
		}

		input = {false, false, false};
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
			input.aPressed = true;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
			input.dPressed = true;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && player.onGround) {
			input.jump = true;
			player.onGround = false;
		}

		if (update_fn.valid()) {
			update_fn(&player, input, dt);
		}

		// physics
		player.applyVelocity({0, 3920 * dt});
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
			player.onGround = true;
		}

		window.clear();
		window.draw(player.shape);
		window.display();

		dt = clock.restart().asSeconds();
		timeSinceLastReload += dt;
	}

	return 0;
}
