#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <list>
#include <cstdint>
#include <vector>

enum class ConfigKey : uint8_t {
    ROM         = 0,
    LINK_MASTER = 1,
    SPEED       = 2,
    EMU_MODE    = 3,
    LINK_PORT   = 4,
    LINK_TYPE   = 5,
    LINK_ADDR   = 6,
    LINK_ENABLE = 7,
};

enum class EmuMode : uint8_t {
    AUTO = 0,
    CGB  = 1,
    DMG  = 2,
};

enum class EmuSpeed : uint8_t {
    NORMAL = 0,
    _2X    = 1,
    FREE   = 2,
};

enum class LinkType : uint8_t {
    SOCKET = 0,
    PIPE   = 1,
};

class Configuration {
public:
    ~Configuration() = default;

    Configuration(const Configuration&) = delete;
    Configuration & operator=(const Configuration&) = delete;
    Configuration(const Configuration&&) = delete;
    Configuration(Configuration&&) = delete;

    static inline Configuration & instance() { return s_instance; }

    inline void parse() { parse(DEFAULT_FILE); }
    void parse(const std::string & file);

    inline void save() const { save(DEFAULT_FILE); }
    void save(const std::string & file) const;

    std::string str() const;

    static std::string toString(ConfigKey key);

    static std::string getString(ConfigKey key, std::string def = "");
    static int getInt(ConfigKey key, int def = 0);
    static bool getBool(ConfigKey key, bool def = false);

    ////////////////////////////////////////////////////////////////////////////
    class SettingValue {
    public:
        virtual ~SettingValue() = default;

        virtual inline std::string toString() const { return m_value; }

        virtual inline bool toBool() const { return false; }
        virtual inline int  toInt()  const { return 0;     }

        inline std::string type() const { return m_type; }

        std::string output(ConfigKey key) const
        {
            std::stringstream stream;
            stream << std::to_string(uint8_t(key))
                   << ":" << m_type
                   << ":" << m_value;
            return stream.str();
        }

        static constexpr const char *TYPE_INT    = "int";
        static constexpr const char *TYPE_STRING = "string";
        static constexpr const char *TYPE_BOOL   = "bool";

    protected:
        explicit SettingValue(const std::string & t, const std::string & value)
            : m_type(t), m_value(value) { }

        std::string m_type;
        std::string m_value;
    };
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    class IntValue : public SettingValue {
    public:
        IntValue(const std::string & value)
            : SettingValue(TYPE_INT, value)
        {
            std::istringstream(value) >> m_iValue;
        }
        IntValue(int value) : IntValue(std::to_string(value)) { }

        ~IntValue() = default;

        inline bool toBool() const override { return (0 == m_iValue); }
        inline int  toInt()  const override { return m_iValue;        }
    private:
        int m_iValue;
    };
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    class StringValue : public SettingValue {
    public:
        StringValue(const std::string & value)
            : SettingValue(TYPE_STRING, value) { }
        ~StringValue() = default;
    };
    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    class BoolValue : public SettingValue {
    public:
        BoolValue(const std::string & value)
            : SettingValue(TYPE_BOOL, value)
        {
            m_bValue = (value == "1");
        }
        BoolValue(bool value) : BoolValue(std::to_string(value)) { }
        ~BoolValue() = default;

        inline bool toBool() const override { return m_bValue;           }
        inline int  toInt()  const override { return (m_bValue) ? 1 : 0; }
    private:
        bool m_bValue;
    };
    ////////////////////////////////////////////////////////////////////////////

    using Setting = std::shared_ptr<SettingValue>;

    Setting operator[](ConfigKey key) const
    {
        auto iterator = m_settings.find(uint8_t(key));
        return (m_settings.end() == iterator) ? nullptr : iterator->second;
    }

private:
    using ConfigMap = std::unordered_map<uint8_t, Setting>;

    static constexpr const char *DEFAULT_FILE = "gbemu.config";

    static Configuration s_instance;

    static const std::list<ConfigKey> KEYS;

    static const ConfigMap DEFAULT_CONFIG;

    explicit Configuration() : m_settings(DEFAULT_CONFIG) { }

    ConfigMap m_settings;

    static std::vector<std::string> split(const std::string & str);
};

#endif
