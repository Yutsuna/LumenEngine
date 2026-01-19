#include "Window.hpp"

#include <iostream>

LumenEngine::Window::Window ()
{
    std::cout << "Window created." << std::endl;
}

LumenEngine::Window::~Window ()
{
    std::cout << "Window destroyed." << std::endl;
}
