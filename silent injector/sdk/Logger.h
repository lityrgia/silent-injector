#pragma once
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdarg>

class Logger {
public:
    enum class Type {
        Info,
        Input,
        Warning,
        Error
    };

    static void log(Type type, std::string message, ...) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm localTime;
        localtime_s(&localTime, &time);

        std::string levelStr;
        std::string colorCode;

        switch (type) {
        case Type::Info:
            levelStr = "INFO";
            colorCode = "\033[32m";
            break;
        case Type::Warning:
            levelStr = "WARNING";
            colorCode = "\033[33m";
            break;
        case Type::Error:
            levelStr = "ERROR";
            colorCode = "\033[31m"; 
            break;
        case Type::Input:
            levelStr = "INPUT";
            colorCode = "\033[37m";
            break;
        }

        char buffer[1024];

        va_list args;
        va_start(args, message);

        vsnprintf(buffer, sizeof(buffer), message.c_str(), args);

        va_end(args);

        std::cout << colorCode << "[" << std::put_time(&localTime, "%H:%M:%S") << " " << levelStr << "] " << buffer << "\033[0m";
    }
};