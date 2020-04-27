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
        TabButton {
            text: qsTr("Call Stack")
        }
    }

    StackLayout {
        width: parent.width

        currentIndex: m_tabs.currentIndex

        ScrollingList {
            model: Disassembly {
                objectName: "disassembly"
            }
            delegate: Text {
                text: address + ": " + description;
            }
        }

        ScrollingList {
            id: m_trace

            model: ExecutionTrace {
                objectName: "trace"
            }
            delegate: Component { Item {
                height: 55
                width: parent.width

                Column {
                Text { text: "<b>" + index + "</b> | " + address + ": " + description; }
                Text { text: "    " + status; }
                Text { text: "    " + registers; }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        m_trace.currentIndex = parseInt(index, 10) - 1
                    }
                }
            } }
        }

        ScrollingList {
            id: m_callstack

            model: CallStack {
                objectName: "stack"
            }
            delegate: Component { Item {
                height: 20
                width: parent.width

                Column {
                    width: parent.width
                    Text {
                        text: "<b>" + index + "</b> | " + address + ": " + description;
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        m_callstack.currentIndex = parseInt(index, 10) - 1
                    }
                }
            } }
        }
    }
}