/**
 * @file main.cpp
 * @brief Entry point for CryptoTracker application
 * @author Student
 *
 * Initializes GLFW, OpenGL context, and ImGui, then runs the main loop.
 */

#include "AppManager.h"
#include <iostream>

int main()
{
    AppManager app;
    if (!app.init()) {
        std::cerr << "Failed to initialize application\n";
        return -1;
    }
    app.run();
    return 0;
}
