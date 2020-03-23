import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Dialogs 1.3

import GameBoy.Screen 1.0
import GameBoy.Types 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  640
    height: 620
    title: qsTr("GameBoy Trilogy")

    ActionGroup {
        id: m_link
    }

    menuBar : MenuBar {
        Menu {
            title: qsTr("&File")

            MenuItem {
                text: qsTr("&Load ROM...")
                onTriggered: {
                    m_file.visible = true;
                }
            }

            MenuSeparator { }
            MenuItem {
                text: qsTr("&Exit")
                onTriggered: {
                    Qt.quit();
                }
            }
        }

        Menu {
            title: qsTr("&Game Link")

            MenuItem {
                id: m_enabled

                text: qsTr("Enabled")
                checkable: true
                onToggled: {
                    m_screen.link_enable = m_enabled.checked;
                }
            }
            MenuItem {
                id: m_master

                text: qsTr("Master")
                checkable: true
                onToggled: {
                    m_screen.link_master = m_master.checked;
                }
            }

            MenuSeparator { }
            Menu {
                title: qsTr("Type")

                Action {
                    id: m_socket

                    text: qsTr("Socket")
                    checkable: true
                    ActionGroup.group: m_link
                    onTriggered: {
                        if (true === m_socket.checked) {
                            m_screen.link_type = ConsoleLinkType.LINK_SOCKET;
                        }
                    }
                }
                Action {
                    id: m_pipe

                    text: qsTr("Pipe")
                    checkable: true
                    ActionGroup.group: m_link
                    onTriggered: {
                        if (true === m_pipe.checked) {
                            m_screen.link_type = ConsoleLinkType.LINK_PIPE;
                        }
                    }
                }
            }
        }

        Menu {
            title: qsTr("&Help")

            MenuItem {
                text: qsTr("&About")
                onTriggered: {

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

    Component.onCompleted: {
        m_master.checked  = m_screen.link_master;
        m_enabled.checked = m_screen.link_enable;

        var link = m_screen.link_type;
        if (ConsoleLinkType.LINK_SOCKET === link) {
            m_socket.checked = true;
        } else if (ConsoleLinkType.LINK_PIPE === link) {
            m_pipe.checked = true;
        }
    }
}