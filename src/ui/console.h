#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <QObject>
#include <QQmlEngine>

#include <memory>

#include "gameboyinterface.h"

#define QML_ENUM(name) \
    qmlRegisterUncreatableMetaObject( \
        ConsoleConfig::staticMetaObject, \
        "GameBoy.Config", \
        1, 0, \
        #name, \
        "Can't instantiate type " #name \
    );

namespace ConsoleConfig {
    Q_NAMESPACE
    // Make sure that these values match the values in configuration.h.  There is
    // a duplicate declaration so that this can be exposed to QML.  It is an
    // unfortunate implementation, but it is necessary.
    enum LinkType {
        LINK_SOCKET = 0,
        LINK_PIPE   = 1,
    };
    Q_ENUM_NS(LinkType);

    enum EmulationSpeed {
        SPEED_NORMAL = 0,
        SPEED_DOUBLE = 1,
        SPEED_FREE   = 2,
    };
    Q_ENUM_NS(EmulationSpeed);

    enum EmulationMode {
        MODE_AUTO = 0,
        MODE_CGB  = 1,
        MODE_DMG  = 2,
    };
    Q_ENUM_NS(EmulationMode);
}

class Console : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString rom READ getROM WRITE setROM NOTIFY romChanged)
    Q_PROPERTY(bool link_master READ getLinkMaster WRITE setLinkMaster NOTIFY linkMasterChanged)
    Q_PROPERTY(bool link_enable READ getLinkEnable WRITE setLinkEnable NOTIFY linkEnableChanged)
    Q_PROPERTY(ConsoleConfig::LinkType link_type READ getLinkType WRITE setLinkType NOTIFY linkTypeChanged)
    Q_PROPERTY(ConsoleConfig::EmulationSpeed emu_speed READ getSpeed WRITE setSpeed NOTIFY emuSpeedChanged)
    Q_PROPERTY(ConsoleConfig::EmulationMode emu_mode READ getMode WRITE setMode NOTIFY emuModeChanged)

public:
    explicit Console(QObject *parent = nullptr);

    static void registerQML()
    {
        qmlRegisterType<Console>("GameBoy.Console", 1, 0, "GameboyConsole");

        QML_ENUM(LinkType);
        QML_ENUM(EmulationSpeed);
        QML_ENUM(EmulationMode);
    }

    QString getROM() const { return m_rom; }
    void setROM(QString rom);

    bool getLinkMaster() const;
    void setLinkMaster(bool master);

    bool getLinkEnable() const;
    void setLinkEnable(bool enable);

    void setLinkType(ConsoleConfig::LinkType link);
    ConsoleConfig::LinkType getLinkType() const;

    void setSpeed(ConsoleConfig::EmulationSpeed speed);
    ConsoleConfig::EmulationSpeed getSpeed() const;

    void setMode(ConsoleConfig::EmulationMode mode);
    ConsoleConfig::EmulationMode getMode() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void step();

    void check();

    void reset(const std::string & filename);

    std::unique_ptr<GameBoyInterface> & get() { return m_console; }

signals:
    void romChanged();
    void linkMasterChanged();
    void linkEnableChanged();
    void linkTypeChanged();
    void emuSpeedChanged();
    void emuModeChanged();

    void paused();
    void resumed();
    void stepped();

private:
    Console(const Console &) = delete;
    Console(const Console &&) = delete;
    Console(Console &&) = delete;
    Console & operator=(const Console &) = delete;

    std::unique_ptr<GameBoyInterface> m_console;

    QString m_rom;
};

#endif
