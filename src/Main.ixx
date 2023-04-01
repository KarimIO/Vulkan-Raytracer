import <iostream>;

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
		std::cerr << e.what() << "\n";
	}

	return 0;
}