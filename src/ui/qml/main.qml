import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0

ApplicationWindow {
    id: m_root
    visible: true
    width:  800
	height: 500
    title: "GBC"
    
	Canvas {
		id: canvas
		width: 800
		height: 500
		
		onPaint: {
			console.log("Canvas Paint Start");
			
			var context = canvas.getContext('2d');
			
			var pixels = context.createImageData(160, 144);
			
			for (var i = 0; i < pixels.width; i++) {
				for (var j = 0; j < pixels.height; j++) {
					var index = ((i * pixels.height) + j) * 4;			
					
					pixels.data[index + 0] = 0xFF;
					pixels.data[index + 1] = 0x00;
					pixels.data[index + 2] = 0x00;
					pixels.data[index + 3] = 0xFF;
				}
			}
			
			
			console.log(pixels.data.length, " ", pixels.width, " ", pixels.height);
			
			context.drawImage(pixels, 50, 50);
		}
	}
}