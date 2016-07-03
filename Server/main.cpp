#include <iostream>
#include <thread>
#include "../ServerLogic/Server.h"

int main() 
{
	Server instance;

	if (!instance.Init())
		return -1;

	std::thread MainThread([&instance]() {
		instance.Run();
	});

	std::cout << "Running..." << std::endl;
	getchar();

	instance.Pause();
	MainThread.join();

	return 0;
}