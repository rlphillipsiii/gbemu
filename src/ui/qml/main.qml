import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0

import GameBoy.Screen 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  640
    height: 576
    title: "GBC"
    
    Screen {
        id: screen
        focus: true
        
        width:  640
        height: 576
    }
}