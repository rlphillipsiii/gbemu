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
	}
	
	Canvas {
		id: canvas
		width: 800
		height: 500
		
		onPaint: {
			console.log("Canvas Paint Start");
			
			var context = canvas.getContext('2d');
			
			var pixels = context.createImageData(160, 144);
			for (var i = 0; i < pixels.data.length; i++) {
				pixels.data[i] = screen.at(i);
			}
			
			console.log(screen.length(), " : " , pixels.data.length, " ", pixels.width, " ", pixels.height);
			
			context.drawImage(pixels, 50, 50);
			
			console.log("Canvas Paint Finished");
		}
	}
}