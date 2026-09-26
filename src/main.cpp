#include <exception>
#include <iostream>

#include "app/ToyRoomApp.h"
#include "core/Paths.h"

int main(int argc, char* argv[])
{
	Paths::init(argv[0]);
	try {
		ToyRoomApp app(LaunchOptions::Parse(argc, argv));
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
