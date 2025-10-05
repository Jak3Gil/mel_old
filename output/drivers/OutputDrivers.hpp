#pragma once

#include "../OutputManager.hpp"
#include <iostream>
#include <fstream>

namespace melvin {

/**
 * TextDriver - Handles text output
 */
class TextDriver : public OutputDriver {
public:
    TextDriver() = default;
    ~TextDriver() override = default;

    bool can_handle(OutputType type) const override {
        return type == OutputType::TEXT || type == OutputType::ABSTRACTION;
    }

    bool output(const OutputIntent& intent) override {
        std::cout << intent.content << std::endl;
        return true;
    }

    std::string get_driver_name() const override {
        return "TextDriver";
    }
};

/**
 * FileDriver - Handles file output
 */
class FileDriver : public OutputDriver {
public:
    FileDriver(const std::string& filename) : filename_(filename) {}
    ~FileDriver() override = default;

    bool can_handle(OutputType type) const override {
        return type == OutputType::TEXT || type == OutputType::ABSTRACTION;
    }

    bool output(const OutputIntent& intent) override {
        std::ofstream file(filename_, std::ios::app);
        if (!file.is_open()) {
            return false;
        }
        
        file << intent.content << std::endl;
        file.close();
        return true;
    }

    std::string get_driver_name() const override {
        return "FileDriver";
    }

private:
    std::string filename_;
};

/**
 * SpeechDriver - Handles speech output (placeholder)
 */
class SpeechDriver : public OutputDriver {
public:
    SpeechDriver() = default;
    ~SpeechDriver() override = default;

    bool can_handle(OutputType type) const override {
        return type == OutputType::SPEECH;
    }

    bool output(const OutputIntent& intent) override {
        // Placeholder for speech synthesis
        std::cout << "[SPEECH] " << intent.content << std::endl;
        return true;
    }

    std::string get_driver_name() const override {
        return "SpeechDriver";
    }
};

/**
 * ActionDriver - Handles action output (placeholder)
 */
class ActionDriver : public OutputDriver {
public:
    ActionDriver() = default;
    ~ActionDriver() override = default;

    bool can_handle(OutputType type) const override {
        return type == OutputType::ACTION;
    }

    bool output(const OutputIntent& intent) override {
        // Placeholder for action execution
        std::cout << "[ACTION] " << intent.content << std::endl;
        return true;
    }

    std::string get_driver_name() const override {
        return "ActionDriver";
    }
};

} // namespace melvin
