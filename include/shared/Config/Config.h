#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class Config {
public:
    static Config& Get();

    bool Load(const std::string& file);
    void Reload();

    std::string GetString(const std::string& key, const std::string& def = "");
    int GetInt(const std::string& key, int def = 0);
    int64_t GetInt64(const std::string& key, int64_t def = 0);
    float GetFloat(const std::string& key, float def = 0.0f);
    bool GetBool(const std::string& key, bool def = false);
    std::vector<std::string> GetStringList(const std::string& key);

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::unordered_map<std::string, std::string> _values;
    std::string _filename;
};
