#include <QCoreApplication>

#include <memory>
#include <string>

#include "configuration.h"
#include "console.h"

using std::string;

Console::Console(QObject *parent)
    : QObject(parent)
{

}

void Console::start()
{
    QStringList args = QCoreApplication::arguments();

    string filename = (args.size() < 2) ?
        Configuration::getString(ConfigKey::ROM) : args.at(1).toStdString();

    m_rom = QString::fromStdString(filename);
    reset(filename);
}

void Console::pause()
{
    if (m_console) {
        m_console->pause();

        emit paused();
    }
}

void Console::resume()
{
    if (m_console) {
        m_console->resume();

        emit resumed();
    }
}

void Console::step()
{
    if (m_console) {
        m_console->pause();
        m_console->step();

        printf("%s\n", m_console->trace().back().str().c_str());
        emit stepped();
    }
}

void Console::check()
{
    if (m_console && m_console->paused()) {
        emit paused();
    }
}

void Console::setLinkMaster(bool master)
{
    Configuration::updateBool(ConfigKey::LINK_MASTER, master);
}
bool Console::getLinkMaster() const
{
    return Configuration::getBool(ConfigKey::LINK_MASTER);
}

void Console::setLinkType(ConsoleConfig::LinkType type)
{
    Configuration::updateInt(ConfigKey::LINK_TYPE, type);
}
ConsoleConfig::LinkType Console::getLinkType() const
{
    return Configuration::getEnum<ConsoleConfig::LinkType>(ConfigKey::LINK_TYPE);
}

void Console::setSpeed(ConsoleConfig::EmulationSpeed speed)
{
    Configuration::updateInt(ConfigKey::SPEED, speed);
}
ConsoleConfig::EmulationSpeed Console::getSpeed() const
{
    return Configuration::getEnum<ConsoleConfig::EmulationSpeed>(ConfigKey::SPEED);
}

void Console::setMode(ConsoleConfig::EmulationMode mode)
{
    Configuration::updateInt(ConfigKey::EMU_MODE, mode);
}
ConsoleConfig::EmulationMode Console::getMode() const
{
    return Configuration::getEnum<ConsoleConfig::EmulationMode>(ConfigKey::EMU_MODE);
}

void Console::setLinkEnable(bool enable)
{
    Configuration::updateBool(ConfigKey::LINK_ENABLE, enable);
}
bool Console::getLinkEnable() const
{
    return Configuration::getBool(ConfigKey::LINK_ENABLE);
}

void Console::stop()
{
    if (m_console) {
        m_console->stop();
    }
    m_console.reset();
}

void Console::setROM(QString rom)
{
    string filename = QUrl(rom).toLocalFile().toStdString();
    reset(filename);
}

void Console::reset()
{
    string filename = m_rom.toStdString();
    reset(filename);
}

void Console::reset(const string & path)
{
    stop();

    m_console = GameBoyInterface::Instance();
    assert(m_console);

#ifdef WIN32
    string filename = path;

    size_t index;
    while ((index = filename.find("/")) != string::npos) {
        filename.replace(index, 1, "\\");
    }
#else
    const string & filename = path;
#endif

    if (!filename.empty()) {
        Configuration::updateString(ConfigKey::ROM, filename);
    }

    m_console->load(filename);
    m_console->start();
}
