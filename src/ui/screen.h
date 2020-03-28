#ifndef _SCREEN_H
#define _SCREEN_H

#include <QtQuick/QQuickPaintedItem>
#include <QObject>
#include <QImage>
#include <QTimer>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "gameboyinterface.h"

#define QML_ENUM(name) \
    qmlRegisterUncreatableMetaObject( \
        ScreenTypes::staticMetaObject, \
        "GameBoy.Types", \
        1, 0, \
        #name, \
        "Can't instantiate type " #name \
    );

namespace ScreenTypes {
    Q_NAMESPACE
    // Make sure that these values match the values in configuration.h.  There is
    // a duplicate declaration so that this can be exposed to QML.  It is an
    // unfortunate implementation, but it is necessary.
    enum ConsoleLinkType {
        LINK_SOCKET = 0,
        LINK_PIPE   = 1,
    };
    Q_ENUM_NS(ConsoleLinkType);

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

class Screen : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(QString rom READ getROM WRITE setROM NOTIFY romChanged)
    Q_PROPERTY(bool link_master READ getLinkMaster WRITE setLinkMaster NOTIFY linkMasterChanged)
    Q_PROPERTY(bool link_enable READ getLinkEnable WRITE setLinkEnable NOTIFY linkEnableChanged)
    Q_PROPERTY(ScreenTypes::ConsoleLinkType link_type READ getLinkType WRITE setLinkType NOTIFY linkTypeChanged)
    Q_PROPERTY(ScreenTypes::EmulationSpeed emu_speed READ getSpeed WRITE setSpeed NOTIFY emuSpeedChanged)
    Q_PROPERTY(ScreenTypes::EmulationMode emu_mode READ getMode WRITE setMode NOTIFY emuModeChanged)

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen();

    void paint(QPainter *painter) override;

    QString getROM() const { return m_rom; }
    void setROM(QString rom);

    bool getLinkMaster() const;
    void setLinkMaster(bool master);

    bool getLinkEnable() const;
    void setLinkEnable(bool enable);

    void setLinkType(ScreenTypes::ConsoleLinkType link);
    ScreenTypes::ConsoleLinkType getLinkType() const;

    void setSpeed(ScreenTypes::EmulationSpeed speed);
    ScreenTypes::EmulationSpeed getSpeed() const;

    void setMode(ScreenTypes::EmulationMode mode);
    ScreenTypes::EmulationMode getMode() const;

    static void registerQML()
    {
        qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");

        QML_ENUM(ConsoleLinkType);
        QML_ENUM(EmulationSpeed);
        QML_ENUM(EmulationMode);
    }

public slots:
    void onTimeout();
    void stop();

signals:
    void romChanged();
    void linkMasterChanged();
    void linkEnableChanged();
    void linkTypeChanged();
    void emuSpeedChanged();
    void emuModeChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static constexpr int REFRESH_TIMEOUT = 33;

    static const std::unordered_map<int, GameBoyInterface::JoyPadButton> BUTTON_MAP;

    Screen(const Screen &) = delete;
    Screen(const Screen &&) = delete;
    Screen(Screen &&) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QImage m_canvas;

    std::shared_ptr<GameBoyInterface> m_console;

    bool m_stopped;

    QTimer m_timer;

    QString m_rom;
};

#endif
