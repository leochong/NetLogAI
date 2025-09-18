#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <manifest.json>\n";
        return 1;
    }

    try {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open " << argv[1] << "\n";
            return 1;
        }

        nlohmann::json manifest;
        file >> manifest;

        std::cout << "Manifest parsing successful!\n";
        std::cout << "Plugin name: " << manifest["name"] << "\n";
        std::cout << "Version: " << manifest["version"] << "\n";
        std::cout << "Type: " << manifest["type"] << "\n";
        std::cout << "Author: " << manifest["author"]["name"] << "\n";

        if (manifest.contains("capabilities")) {
            std::cout << "Capabilities: ";
            for (const auto& cap : manifest["capabilities"]) {
                std::cout << cap << " ";
            }
            std::cout << "\n";
        }

        std::cout << "Manifest validation passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing manifest: " << e.what() << "\n";
        return 1;
    }
}