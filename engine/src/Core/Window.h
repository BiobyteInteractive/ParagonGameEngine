#pragma once
#ifndef WINDOW_H
#define WINDOW_H

#define DllExport __declspec( dllexport )

#include <GLFW/glfw3.h>

#include <string>

class DllExport Window {
    public:
        Window(int screenWidth, int screenHeight, std::string windowTitle);
        ~Window();
        GLFWwindow* GetWindowHandle();
    private:
        GLFWwindow* m_Window;
};

#endif