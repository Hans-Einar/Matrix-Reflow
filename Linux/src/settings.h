#pragma once
#include "mmcore.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace reflow {
struct Settings {
    MMSettings rain = mm_settings_default();
    int fps_limit = 60;
};
enum class FieldType { Number, Integer, Boolean };
struct SettingField {
    const char* key;
    const char* label;
    const char* group;
    FieldType type;
    double low, high;
    std::function<double(const Settings&)> get;
    std::function<void(Settings&, double)> put;
};
const std::vector<SettingField>& setting_fields();
const SettingField* setting_field(const std::string& key);
void set_setting(Settings&, const std::string& key, const std::string& value);
void validate_settings(const Settings&);
std::string setting_number(double);
std::string effective_settings(const Settings&);
std::vector<std::string> settings_arguments(const Settings&);
struct Profile { std::string name; Settings settings; };
struct Profiles {
    std::array<Profile, 5> profiles;
    int selected = 1; // stable slot 1..5; names can change
    Profiles();
};
std::string settings_path();
Profiles load_profiles(const std::string& path);
void save_profiles(const Profiles&, const std::string& path);
}
