#include "stdafx.h"
#include "injector.h"

#include <fstream>

std::vector<std::string> Split(std::string& s) {
    std::vector<std::string> argv;
    int left = -1;
    int right = 0;
    while (left != s.size()) {
        while (right < s.size() && s[right] != ' ')
            ++right;
        argv.push_back(std::string{ s.begin() + 1 + left, s.begin() + right });
        left = right++;
    }
    return argv;
}

int main()
{
    try {
        const char* config_path = "config.cfg";
        if (!std::filesystem::exists(config_path))
            throw std::runtime_error("No config specified\n");

        std::ifstream config(config_path);
        std::string game;
        std::getline(config, game);

        std::string line;
        while (std::getline(config, line)) {
            auto argv = Split(line);
            if (argv[0] == "mono-dep") {
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading dependency " << argv[1].data() << ": ";
                Injector::InjectAssembly(game.data(), full_dll_path.data());
                std::cout << "OK\n";
            }
            if (argv[0] == "mono-ex") {
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading mod " << argv[1].data() << ": ";
                Injector::InjectAssemblyEx(game.data(), full_dll_path.data(), argv[2].data(), argv[3].data(), argv[4].data());
                std::cout << "OK\n";
            }
            if (argv[0] == "native") {
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading dll " << argv[1].data() << ": ";
                Injector::InjectDll(game.data(), full_dll_path.data());
                std::cout << "OK\n";
            }
            /*if (argv[0] == "mono-ej") {
                std::cout << "Unloading mod " << argv[1].data() << ": ";
                Injector::EjectAssemblyEx(game.data(), argv[1].data(), argv[2].data(), argv[3].data(), argv[4].data());
                std::cout << "OK\n";
            }*/
        }
        std::cout << "All modules loaded\n";
    }
    catch (const std::runtime_error& error) {
        std::cout << error.what() << "\n";
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
    }
    system("pause");
    return 0;
}