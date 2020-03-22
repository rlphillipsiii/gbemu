import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.11
import QtQuick.Dialogs 1.3

import GameBoy.Screen 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  640
    height: 620
    title: qsTr("GameBoy Trilogy")

    menuBar : MenuBar {
        Menu {
            title: qsTr("&File")

            Action {
                text: qsTr("&Load ROM...")
                onTriggered: {
                    m_file.visible = true;
                }
            }

            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: {
                    Qt.quit();
                }
            }
        }
    }

    Screen {
        id: m_screen
        focus: true

        width:  640
        height: 576
    }

    FileDialog {
        id: m_file
        title: "Load ROM..."
        visible: false
        selectMultiple: false
        nameFilters: [ "GameBoy Roms (*.gbc *.gb)", "All files (*)" ]
        onAccepted: {
            m_screen.rom = m_file.fileUrl;
        }
    }
}