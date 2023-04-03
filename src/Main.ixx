import std.core;

import RaytracerEngine;

int main() {
	try {
		RaytracerEngine raytracer;

		if (!raytracer.Initialize()) {
			return 1;
		}
	
		raytracer.Run();
	}
	catch (std::runtime_error e) {
		std::cerr << "Fatal Error: " << e.what() << "\n";
	}

	return 0;
}