import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0

import GameBoy.Screen 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  800
	height: 500
    title: "GBC"
    
	Screen {
		id: screen
		
		width:  800
		height: 500
	}
}