#include <sol/sol.hpp>
#include <iostream>

int main() {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	lua.script("print('Lua works')");

	return 0;
}
