import QtQuick 2.14
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.14

import GameBoy.Disassembly 1.0
import GameBoy.Console 1.0
import GameBoy.Trace 1.0

ColumnLayout {
    property GameboyConsole game_console

    Keys.forwardTo: [ m_trace ]

    TabBar {
        id: m_tabs
        width: parent.width

        TabButton {
            text: qsTr("Disassembly")
        }

        TabButton {
            text: qsTr("Trace")
        }
    }

    StackLayout {
        width: parent.width

        currentIndex: m_tabs.currentIndex

        ListView {
            width:  parent.width
            height: parent.height

            model: Disassembly {
                objectName: "disassembly"
            }
            delegate: Text {
                text: address + ": " + description;
            }

            ScrollBar.vertical: ScrollBar {}
        }

        ListView {
            id: m_trace

            width:  parent.width
            height: parent.height

            keyNavigationEnabled: true

            model: ExecutionTrace {
                objectName: "trace"
            }
            delegate: Column {
                Text {
                    text: "<b>" + index + "</b> | " + address + ": " + description;
                }
                Text {
                    text: "    " + status;
                }
                Text {
                    text: "    " + registers;
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}