#include <sol/sol.hpp>
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

// Box2D unit conversion (pixels per meter)
constexpr float PPM = 100.0f; // 100 pixels = 1 meter
constexpr float MPP = 1.0f / PPM; // meters per pixel

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

// Debug Draw for Box2D visualization
class DebugDraw : public b2Draw {
private:
	sf::RenderWindow* window;

public:
	explicit DebugDraw(sf::RenderWindow* win) : window(win) {
		SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_aabbBit | b2Draw::e_centerOfMassBit);
	}

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override {
		sf::ConvexShape polygon(vertexCount);
		for (int i = 0; i < vertexCount; i++) {
			polygon.setPoint(i, sf::Vector2f(vertices[i].x * PPM, vertices[i].y * PPM));
		}
		polygon.setFillColor(sf::Color::Transparent);
		polygon.setOutlineColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                                   static_cast<std::uint8_t>(color.g * 255),
		                                   static_cast<std::uint8_t>(color.b * 255),
		                                   200));
		polygon.setOutlineThickness(-1.0f); // Negative = draw inside
		window->draw(polygon);
	}

	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override {
		sf::ConvexShape polygon(vertexCount);
		for (int i = 0; i < vertexCount; i++) {
			polygon.setPoint(i, sf::Vector2f(vertices[i].x * PPM, vertices[i].y * PPM));
		}
		polygon.setFillColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                                static_cast<std::uint8_t>(color.g * 255),
		                                static_cast<std::uint8_t>(color.b * 255),
		                                60));
		polygon.setOutlineColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                                   static_cast<std::uint8_t>(color.g * 255),
		                                   static_cast<std::uint8_t>(color.b * 255),
		                                   255));
		polygon.setOutlineThickness(-1.0f); // Negative = draw inside
		window->draw(polygon);
	}

	void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override {
		sf::CircleShape circle(radius * PPM);
		circle.setPosition(sf::Vector2f(center.x * PPM - radius * PPM, center.y * PPM - radius * PPM));
		circle.setFillColor(sf::Color::Transparent);
		circle.setOutlineColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                                  static_cast<std::uint8_t>(color.g * 255),
		                                  static_cast<std::uint8_t>(color.b * 255),
		                                  200));
		circle.setOutlineThickness(-1.0f); // Negative = draw inside
		window->draw(circle);
	}

	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override {
		sf::CircleShape circle(radius * PPM);
		circle.setPosition(sf::Vector2f(center.x * PPM - radius * PPM, center.y * PPM - radius * PPM));
		circle.setFillColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                               static_cast<std::uint8_t>(color.g * 255),
		                               static_cast<std::uint8_t>(color.b * 255),
		                               60));
		circle.setOutlineColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                                  static_cast<std::uint8_t>(color.g * 255),
		                                  static_cast<std::uint8_t>(color.b * 255),
		                                  255));
		circle.setOutlineThickness(-1.0f); // Negative = draw inside
		window->draw(circle);
	}

	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override {
		sf::Vertex line[2];
		line[0].position = sf::Vector2f(p1.x * PPM, p1.y * PPM);
		line[0].color = sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                          static_cast<std::uint8_t>(color.g * 255),
		                          static_cast<std::uint8_t>(color.b * 255));
		line[1].position = sf::Vector2f(p2.x * PPM, p2.y * PPM);
		line[1].color = sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                          static_cast<std::uint8_t>(color.g * 255),
		                          static_cast<std::uint8_t>(color.b * 255));
		window->draw(line, 2, sf::PrimitiveType::Lines);
	}

	void DrawTransform(const b2Transform& xf) override {
		const float axisScale = 0.4f;
		b2Vec2 p1 = xf.p;

		// X axis (red)
		b2Vec2 p2 = p1 + axisScale * xf.q.GetXAxis();
		DrawSegment(p1, p2, b2Color(1, 0, 0));

		// Y axis (green)
		p2 = p1 + axisScale * xf.q.GetYAxis();
		DrawSegment(p1, p2, b2Color(0, 1, 0));
	}

	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override {
		sf::CircleShape point(size);
		point.setPosition(sf::Vector2f(p.x * PPM - size, p.y * PPM - size));
		point.setFillColor(sf::Color(static_cast<std::uint8_t>(color.r * 255),
		                              static_cast<std::uint8_t>(color.g * 255),
		                              static_cast<std::uint8_t>(color.b * 255)));
		window->draw(point);
	}
};

struct Player {
	bool onGround = false;
	sf::RectangleShape shape;
	b2Body* body = nullptr;
	int jumpCount = 0;
	bool coyoteJumpAvailable = true;

	// Getters that read directly from Box2D
	sf::Vector2f getPosition() const {
		if (body) {
			b2Vec2 position = body->GetPosition();
			return sf::Vector2f(position.x * PPM, position.y * PPM);
		}
		return sf::Vector2f(0, 0);
	}

	sf::Vector2f getVelocity() const {
		if (body) {
			b2Vec2 velocity = body->GetLinearVelocity();
			return sf::Vector2f(velocity.x * PPM, velocity.y * PPM);
		}
		return sf::Vector2f(0, 0);
	}

	// Setters that write directly to Box2D
	void setPosition(const sf::Vector2f& v) {
		if (body) {
			body->SetTransform(b2Vec2(v.x * MPP, v.y * MPP), body->GetAngle());
		}
	}

	void setVelocity(const sf::Vector2f& v) {
		if (body) {
			body->SetLinearVelocity(b2Vec2(v.x * MPP, v.y * MPP));
		}
	}

	void applyVelocity(const sf::Vector2f& v) {
		if (body) {
			b2Vec2 currentVel = body->GetLinearVelocity();
			body->SetLinearVelocity(b2Vec2(currentVel.x + v.x * MPP, currentVel.y + v.y * MPP));
		}
	}

	void move(const sf::Vector2f& v) {
		if (body) {
			b2Vec2 currentPos = body->GetPosition();
			body->SetTransform(b2Vec2(currentPos.x + v.x * MPP, currentPos.y + v.y * MPP), body->GetAngle());
		}
	}

	void updateRenderPosition() {
		if (body) {
			b2Vec2 position = body->GetPosition();
			sf::Vector2f shapeSize = shape.getSize();
			// Calculate top-left corner, then round the FINAL position
			float pixelX = (position.x * PPM) - (shapeSize.x / 2.0f);
			float pixelY = (position.y * PPM) - (shapeSize.y / 2.0f);
			// Round to nearest pixel
			shape.setPosition(sf::Vector2f(std::round(pixelX), std::round(pixelY)));
		}
	}
};

struct Platform {
	sf::RectangleShape shape;
	b2Body* body = nullptr;

	// Getters that read directly from Box2D
	sf::Vector2f getPosition() const {
		if (body) {
			b2Vec2 position = body->GetPosition();
			return sf::Vector2f(position.x * PPM, position.y * PPM);
		}
		return sf::Vector2f(0, 0);
	}

	sf::Vector2f getVelocity() const {
		if (body) {
			b2Vec2 velocity = body->GetLinearVelocity();
			return sf::Vector2f(velocity.x * PPM, velocity.y * PPM);
		}
		return sf::Vector2f(0, 0);
	}

	// Setters that write directly to Box2D
	void setPosition(const sf::Vector2f& v) {
		if (body) {
			body->SetTransform(b2Vec2(v.x * MPP, v.y * MPP), body->GetAngle());
		}
	}

	void setVelocity(const sf::Vector2f& v) {
		if (body) {
			body->SetLinearVelocity(b2Vec2(v.x * MPP, v.y * MPP));
		}
	}

	void applyVelocity(const sf::Vector2f& v) {
		if (body) {
			b2Vec2 currentVel = body->GetLinearVelocity();
			body->SetLinearVelocity(b2Vec2(currentVel.x + v.x * MPP, currentVel.y + v.y * MPP));
		}
	}

	void move(const sf::Vector2f& v) {
		if (body) {
			b2Vec2 currentPos = body->GetPosition();
			body->SetTransform(b2Vec2(currentPos.x + v.x * MPP, currentPos.y + v.y * MPP), body->GetAngle());
		}
	}

	void updateRenderPosition() {
		if (body) {
			b2Vec2 position = body->GetPosition();
			sf::Vector2f shapeSize = shape.getSize();
			// Calculate top-left corner with 1px extra on all sides to hide collision margin
			float pixelX = (position.x * PPM) - (shapeSize.x / 2.0f);
			float pixelY = (position.y * PPM) - (shapeSize.y / 2.0f);
			// Round to nearest pixel
			shape.setPosition(sf::Vector2f(std::round(pixelX), std::round(pixelY)));
		}
	}

	static Platform* makePlatform(sf::Vector2f position, sf::Vector2f size, b2World& world, float friction, float bounciness) {
		Platform* platform = new Platform;
		// Make visual shape larger to hide Box2D collision margin (2px on each side)
		platform->shape.setSize(sf::Vector2f(size.x + 2.0f, size.y + 2.0f));
		platform->shape.setFillColor(sf::Color::White);

		// Calculate physics center from requested position and physics size
		sf::Vector2f halfSize = size / 2.f;
		float centerX = std::round(position.x + halfSize.x);
		float centerY = std::round(position.y + halfSize.y);

		// Create Box2D body at rounded center position
		b2BodyDef bodyDef;
		bodyDef.position.Set(centerX * MPP, centerY * MPP);
		platform->body = world.CreateBody(&bodyDef);

		// Create collision box with physics size (not visual size)
		b2PolygonShape box;
		box.SetAsBox(halfSize.x * MPP, halfSize.y * MPP);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &box;
		fixtureDef.friction = friction; // Lower friction to reduce visual gaps
		fixtureDef.restitution = bounciness; // No bounce
		platform->body->CreateFixture(&fixtureDef);

		return platform;
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
		"a_pressed", &Input::aPressed,
		"d_pressed", &Input::dPressed,
		"jump", &Input::jump
	);
	lua.new_usertype<Player>("Player",
		"pos", sol::property(&Player::getPosition, &Player::setPosition),
		"vel", sol::property(&Player::getVelocity, &Player::setVelocity),
		"on_ground", &Player::onGround,
		"jump_count", &Player::jumpCount,
		"coyote_jump_available", &Player::coyoteJumpAvailable,
		"apply_velocity", &Player::applyVelocity,
		"set_velocity", &Player::setVelocity,
		"move", &Player::move,
		"set_position", &Player::setPosition
	);

	// Create Box2D world
	std::cout << "Creating Box2D world" << std::endl;
	b2Vec2 gravity(0.0f, 39.2f); // 9.8 m/s^2 * 4 for game feel
	b2World world(gravity);

	sf::RenderWindow window(sf::VideoMode({1280, 720}), "SFML works");

	// Setup debug draw
	DebugDraw debugDraw(&window);
	world.SetDebugDraw(&debugDraw);
	bool showDebugDraw = true; // Toggle with F1

	bool running = true;

	Player player;
	player.shape = sf::RectangleShape({50, 100});
	player.shape.setFillColor(sf::Color::Red);

	// Create player body in Box2D
	b2BodyDef playerBodyDef;
	playerBodyDef.type = b2_dynamicBody;

	float startX = 500.0f * MPP;
	float startY = 30.0f * MPP;
	playerBodyDef.position.Set(startX, startY);
	playerBodyDef.fixedRotation = true; // Prevent rotation
	player.body = world.CreateBody(&playerBodyDef);

	b2PolygonShape playerBox;
	playerBox.SetAsBox(25.0f * MPP, 50.0f * MPP); // Half-widths

	b2FixtureDef playerFixtureDef;
	playerFixtureDef.shape = &playerBox;
	playerFixtureDef.density = 1.0f;
	playerFixtureDef.friction = 0.6f; // Lower friction to reduce sticking on side collisions
	playerFixtureDef.restitution = 0.0f; // No bounce
	player.body->CreateFixture(&playerFixtureDef);

	// Initialize player render position
	player.updateRenderPosition();

	std::vector<Platform*> platforms;

	platforms.push_back(Platform::makePlatform({100.f, 700.f}, {1000.f, 10.f}, world, 0.2f, 0.f));
	platforms.push_back(Platform::makePlatform({600.f, 650.f}, {200.f, 30.f}, world, 0.2f, 0.f));
	platforms.push_back(Platform::makePlatform({100.f, 300.f}, {30.f, 400.f}, world, 0.2f, 0.f));

	for (auto* platform : platforms) {
		platform->updateRenderPosition();
	}

	Input input;

	sf::Clock clock;
	sf::Clock coyoteJumpClock;
	float dt = clock.restart().asSeconds();
	float timeSinceLastReload = 0;

	bool prevJumpPressed = false; // Track previous frame's jump state for edge detection

	// Load Lua script initially
	try {
		load_script();
		last_write_time = fs::last_write_time("../../scripts/player.lua");
		std::cout << "Lua script loaded successfully" << std::endl;
	} catch (const std::exception& e) {
		std::cout << "Error loading Lua script: " << e.what() << "\n";
	}

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
			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
				if (keyPressed->code == sf::Keyboard::Key::F1) {
					showDebugDraw = !showDebugDraw;
					std::cout << "Debug Draw: " << (showDebugDraw ? "ON" : "OFF") << std::endl;
				}
				if (keyPressed->code == sf::Keyboard::Key::F2) {
					// Print position debug info
					b2Vec2 playerPos = player.body->GetPosition();
					sf::Vector2f playerRenderPos = player.shape.getPosition();
					sf::Vector2f playerSize = player.shape.getSize();
					float expectedX = (playerPos.x * PPM) - (playerSize.x / 2.0f);
					float expectedY = (playerPos.y * PPM) - (playerSize.y / 2.0f);
					std::cout << "\n=== Position Debug ===" << std::endl;
					std::cout << "Player Box2D center: (" << playerPos.x * PPM << ", " << playerPos.y * PPM << ")" << std::endl;
					std::cout << "Player expected top-left: (" << expectedX << ", " << expectedY << ")" << std::endl;
					std::cout << "Player SFML pos (rounded): (" << playerRenderPos.x << ", " << playerRenderPos.y << ")" << std::endl;
					for (size_t i = 0; i < platforms.size(); i++) {
						b2Vec2 platformPos = platforms[i]->body->GetPosition();
						sf::Vector2f platformRenderPos = platforms[i]->shape.getPosition();
						sf::Vector2f platformSize = platforms[i]->shape.getSize();
						float expX = (platformPos.x * PPM) - (platformSize.x / 2.0f);
						float expY = (platformPos.y * PPM) - (platformSize.y / 2.0f);
						std::cout << "Platform " << i << " Box2D center: (" << platformPos.x * PPM << ", " << platformPos.y * PPM << ")" << std::endl;
						std::cout << "Platform " << i << " expected top-left: (" << expX << ", " << expY << ")" << std::endl;
						std::cout << "Platform " << i << " SFML pos (rounded): (" << platformRenderPos.x << ", " << platformRenderPos.y << ")" << std::endl;
					}
					std::cout << "==================\n" << std::endl;
				}
			}
		}

		input = {false, false, false};
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
			input.aPressed = true;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
			input.dPressed = true;
		}

		// Jump edge detection - only trigger on button press, not hold
		bool jumpCurrentlyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
		if (jumpCurrentlyPressed && !prevJumpPressed) {
			input.jump = true; // Rising edge - button just pressed
		}
		prevJumpPressed = jumpCurrentlyPressed;

		// Check if on ground using Box2D contact points
		float coyoteJumpElapsed = coyoteJumpClock.getElapsedTime().asSeconds();

		player.onGround = false;
		if (coyoteJumpElapsed > 0.1f) {
			player.coyoteJumpAvailable = false;
		}
		for (b2ContactEdge* ce = player.body->GetContactList(); ce; ce = ce->next) {
			b2Contact* contact = ce->contact;
			if (contact->IsTouching()) {
				b2WorldManifold worldManifold;
				contact->GetWorldManifold(&worldManifold);

				// Get the normal relative to this body
				b2Vec2 normal = worldManifold.normal;
				if (contact->GetFixtureA()->GetBody() != player.body) {
					normal.x = -normal.x;
					normal.y = -normal.y;
				}

				// Check if contact normal points upward (player is standing on something)
				if (normal.y > 0.3f) {
					player.onGround = true;
					coyoteJumpClock.restart();
					player.jumpCount = 0;
					player.coyoteJumpAvailable = true;
					break;
				}
			}
		}

		if (update_fn.valid()) {
			update_fn(&player, input, dt);
		}

		// Apply friction when not moving
		if (!input.aPressed && !input.dPressed) {
			b2Vec2 vel = player.body->GetLinearVelocity();
			vel.x *= 0.9f;
			player.body->SetLinearVelocity(vel);
		}

		// Clamp velocity
		b2Vec2 vel = player.body->GetLinearVelocity();
		vel.x = std::clamp(vel.x, -600.f * MPP, 600.f * MPP);
		vel.y = std::clamp(vel.y, -3920.f * MPP, 3920.f * MPP);
		player.body->SetLinearVelocity(vel);

		// Step Box2D world with more position iterations for accurate collision
		world.Step(dt, 8, 6);

		// Update rendering position from Box2D
		player.updateRenderPosition();
		// ground->updateRenderPosition();


		window.clear(sf::Color(20, 20, 20)); // Dark gray background to see gaps
		for (auto* platform : platforms) {
			window.draw(platform->shape);
		}
		window.draw(player.shape);

		// Draw debug shapes
		if (showDebugDraw) {
			world.DebugDraw();
		}

		window.display();

		dt = clock.restart().asSeconds();
		timeSinceLastReload += dt;
	}

	return 0;
}
