#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "../settings.h"

namespace notifications {
    enum class NotificationType {
        Info,       // Default Red accent color
        Success,    // Green
        Warning,    // Yellow/Orange
        Error       // Red
    };

    struct Notification {
        std::string message;
        NotificationType type;
        float lifetime;
        float current_time;
        float slide_progress;  // 0.0 to 1.0 for slide animation
        float fade_progress;   // 0.0 to 1.0 for fade animation
        int id;
        
        Notification(const std::string& msg, NotificationType t, float life = 6.0f)
            : message(msg), type(t), lifetime(life), current_time(0.0f), 
              slide_progress(0.0f), fade_progress(0.0f), id(0) {}
    };

    // Add a notification to the queue
    // Example usage:
    //   notifications::add("Config saved successfully!", NotificationType::Success);
    //   notifications::add("Failed to load config", NotificationType::Error, 5.0f);
    //   notifications::add("Aimbot enabled", NotificationType::Info);
    void add(const std::string& message, NotificationType type = NotificationType::Info, float lifetime = 3.0f);
    
    // Internal functions - called automatically by render system
    void render();
    void update();
}

