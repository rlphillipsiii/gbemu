import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Window 2.14
import QtQuick.Layouts 1.13

import GameBoy.Screen 1.0
import GameBoy.Disassembly 1.0
import GameBoy.Console 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  1040
    height: 650
    title: qsTr("GameBoy Trilogy")

    GameboyConsole {
        id: m_console
        objectName: "console"

        onPaused: {
            updateFrameRate(0);
        }
    }

    menuBar : GameMenuBar {
        id: m_menu
        game_console: m_console
    }

    RowLayout {
        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_P: m_console.pause();  break;
            case Qt.Key_R: m_console.resume(); break;
            case Qt.Key_S: m_console.step();   break;

            default: break;
            }
        }

        Keys.forwardTo: [ m_screen, m_instructions ]

        focus: true

        ColumnLayout {
            Screen {
                id: m_screen

                width:  640
                height: 576

                onNotify: { m_console.pause(); }
                onFrameRateUpdated: { updateFrameRate(rate); }
            }
            Text { id: m_rate }
        }

        DisassemblyWindow {
            id: m_instructions
            width:  400
            height: 576

            game_console: m_console
        }
    }

    Component.onCompleted: {
        m_menu.load();

        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);

        m_console.start();
        m_screen.start();
    }

    function updateFrameRate(rate) {
        m_rate.text = "<b>Frames Per Second:</b> " + rate;
    }
}