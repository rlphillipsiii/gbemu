// Make sure that the entries in this dictionary match the
// entries that are in the gameboyinterface.h enum definition
const GAME_KEY_MAP  = {
    "KeyA"       : 0x10,
    "KeyB"       : 0x20,
    "KeyF"       : 0x40,
    "Space"      : 0x80,
    "ArrowRight" : 0x01,
    "ArrowLeft"  : 0x02,
    "ArrowUp"    : 0x04,
    "ArrowDown"  : 0x08,
};

const EventType = {
    "KeyUp"   : 0,
    "KeyDown" : 1,
};

const SCREEN_HEIGHT = 144;
const SCREEN_WIDTH  = 160;

class GameScreen {
    constructor(id)
    {
        this.canvas = document.getElementById(id);

        var addr = self.location.host.split(":")[0];

        this.socket = new WebSocket("ws://" + addr + ":1148/");
        this.socket.binaryType = "arraybuffer";

        this.socket.onopen = (event) => {
            this.onOpen(event);
        };
        this.socket.onmessage = (event) => {
            this.onMessage(event);
        };
    }

    onOpen(event)
    {
        console.log("connection established");
    }

    onMessage(event)
    {
        this.canvas.height = SCREEN_HEIGHT;
        this.canvas.width  = SCREEN_WIDTH;
        var context = this.canvas.getContext('2d');

        var img = new ImageData(
            new Uint8ClampedArray(event.data),
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        );

        this.canvas.height *= 4;
        this.canvas.width  *= 4;

        context.putImageData(img, 0, 0);

        context.scale(4, 4);
        context.drawImage(context.canvas, 0, 0);
    }

    onKeyUp(event)
    {
        if (GAME_KEY_MAP[event.code] !== undefined) {
            this.sendEventMessage(EventType.KeyUp, GAME_KEY_MAP[event.code]);
        }
    }

    onKeyDown(event)
    {
        if (GAME_KEY_MAP[event.code] !== undefined) {
            this.sendEventMessage(EventType.KeyDown, GAME_KEY_MAP[event.code]);
        }
    }

    sendEventMessage(event, data)
    {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "event?" + event + "=" + data, true);

        xhr.onload = function() {
            if (204 !== xhr.status) {
                console.log("Unexpected response: " + xhr.status);
            }
        }

        xhr.send();
    }
}
