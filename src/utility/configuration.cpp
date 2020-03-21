#include <string>
#include <list>
#include <fstream>
#include <memory>
#include <vector>
#include <sstream>

#include "configuration.h"
#include "logging.h"

using std::string;
using std::list;
using std::ifstream;
using std::ofstream;
using std::shared_ptr;
using std::vector;
using std::stringstream;

const list<ConfigKey> Configuration::KEYS = {
    ConfigKey::ROM,
    ConfigKey::LINK_MASTER,
    ConfigKey::SPEED,
    ConfigKey::EMU_MODE,
    ConfigKey::LINK_PORT,
    ConfigKey::LINK_TYPE,
    ConfigKey::LINK_ADDR,
    ConfigKey::LINK_ENABLE,
};

const Configuration::ConfigMap Configuration::DEFAULT_CONFIG{
    {
        uint8_t(ConfigKey::ROM),
        Configuration::Setting(new StringValue(""))
    },
    {
        uint8_t(ConfigKey::LINK_MASTER),
        Configuration::Setting(new BoolValue(true))
    },
    {
        uint8_t(ConfigKey::SPEED),
        Configuration::Setting(new IntValue(uint8_t(EmuSpeed::FREE)))
    },
    {
        uint8_t(ConfigKey::EMU_MODE),
        Configuration::Setting(new IntValue(uint8_t(EmuMode::AUTO)))
    },
    {
        uint8_t(ConfigKey::LINK_PORT),
        Configuration::Setting(new IntValue(8008))
    },
    {
        uint8_t(ConfigKey::LINK_TYPE),
        Configuration::Setting(new IntValue(uint8_t(LinkType::SOCKET)))
    },
    {
        uint8_t(ConfigKey::LINK_ADDR),
        Configuration::Setting(new StringValue("127.0.0.1"))
    },
    {
        uint8_t(ConfigKey::LINK_ENABLE),
        Configuration::Setting(new BoolValue(false))
    },
};

Configuration Configuration::s_instance;

vector<string> Configuration::split(const string & str)
{
    const string sep(":");

    string s = str;

    vector<string> tokens;
    for (size_t idx = s.find(sep); idx != string::npos; idx = s.find(sep)) {
        tokens.push_back(s.substr(0, idx));

        s.erase(0, idx + sep.size());
    }

    tokens.push_back(s);
    return tokens;
}

void Configuration::parse(const string & file)
{
    if (file.empty()) { return; }

    ifstream stream(file);
    if (!stream.good()) {
        WARN("Failed to open configuration file: %s\n", file.c_str());
        return;
    }

    string line;
    while (std::getline(stream, line)) {
        if (line.empty() || ('#' == line.at(0))) { continue; }

        vector<string> tokens = split(line);
        if (3 != tokens.size()) { continue; }

        const string & key   = tokens.at(0);
        const string & type  = tokens.at(1);
        const string & value = tokens.at(2);

        auto iterator = m_settings.find(uint8_t(std::stoi(key)));
        if (m_settings.end() == iterator) { continue; }

        Setting setting;
        if (SettingValue::TYPE_STRING == type) {
            setting = Setting(new StringValue(value));
        } else if (SettingValue::TYPE_INT == type) {
            setting = Setting(new IntValue(value));
        } else if (SettingValue::TYPE_BOOL == type) {
            setting = Setting(new BoolValue(value));
        } else {
            WARN("Unknown configuration data type: %s\n", type.c_str());
            continue;
        }

        iterator->second = setting;
    }
}

void Configuration::save(const string & file) const
{
    if (file.empty()) { return; }

    ofstream stream(file);
    if (!stream.good()) {
        WARN("Failed to open configuration file: %s\n", file.c_str());
        return;
    }

    stream << str();
}

string Configuration::str() const
{
    stringstream stream;

    for (const auto & key : KEYS) {
        Setting setting = (*this)[key];
        if (setting) {
            stream << "# " << toString(key) << std::endl;
            stream << setting->output(key) << std::endl;
        }
    }
    return stream.str();
}

string Configuration::toString(ConfigKey key)
{
#define CASE(name) case name: return #name

    switch (key) {
    CASE(ConfigKey::ROM);
    CASE(ConfigKey::LINK_MASTER);
    CASE(ConfigKey::SPEED);
    CASE(ConfigKey::EMU_MODE);
    CASE(ConfigKey::LINK_PORT);
    CASE(ConfigKey::LINK_ADDR);
    CASE(ConfigKey::LINK_TYPE);
    CASE(ConfigKey::LINK_ENABLE);

    default: break;
    }
    return "ConfigKey::UNKNOWN";
}

void Configuration::broadcastUpdate(ConfigKey key)
{
    for (auto & handler : m_handlers) {
        handler.get().onConfigChange(key);
    }
}

////////////////////////////////////////////////////////////////////////////////
string Configuration::getString(ConfigKey key, string def)
{
    Setting setting = Configuration::instance()[key];
    return (setting) ? setting->toString() : def;
}

int Configuration::getInt(ConfigKey key, int def)
{
    Setting setting = Configuration::instance()[key];
    return (setting) ? setting->toInt() : def;
}

bool Configuration::getBool(ConfigKey key, bool def)
{
    Setting setting = Configuration::instance()[key];
    return (setting) ? setting->toBool() : def;
}

bool Configuration::updateString(ConfigKey key, const string & value)
{
    Setting setting = Configuration::instance()[key];
    if (!setting) { return false; }

    if (SettingValue::TYPE_STRING != setting->type()) {
        return false;
    }

    setting->set(value);

    Configuration::instance().broadcastUpdate(key);
    return true;
}

bool Configuration::updateBool(ConfigKey key, bool value)
{
    Setting setting = Configuration::instance()[key];
    if (!setting) { return false; }

    if (SettingValue::TYPE_BOOL != setting->type()) {
        return false;
    }

    setting->set(value);

    Configuration::instance().broadcastUpdate(key);
    return true;
}

bool Configuration::updateInt(ConfigKey key, int value)
{
    Setting setting = Configuration::instance()[key];
    if (!setting) { return false; }

    if (SettingValue::TYPE_INT != setting->type()) {
        return false;
    }

    setting->set(value);

    Configuration::instance().broadcastUpdate(key);
    return true;
}
