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
        this._connected = false;

        this._canvas = document.getElementById(id);

        var addr = self.location.host.split(":")[0];

        this._socket = new WebSocket("ws://" + addr + ":1148/");
        this._socket.binaryType = "arraybuffer";

        this._socket.onopen = (event) => {
            this.onOpen(event);
        };
        this._socket.onclose = (event) => {
            this.onClose(event);
        }
        this._socket.onmessage = (event) => {
            this.onMessage(event);

            this._socket.send("r");
        };

        this._keypress = {
            "KeyA"       : EventType.KeyUp,
            "KeyB"       : EventType.KeyUp,
            "KeyF"       : EventType.KeyUp,
            "Space"      : EventType.KeyUp,
            "ArrowRight" : EventType.KeyUp,
            "ArrowLeft"  : EventType.KeyUp,
            "ArrowUp"    : EventType.KeyUp,
            "ArrowDown"  : EventType.KeyUp,
        };
    }

    onOpen(event)
    {
        this._connected = true;
    }

    onClose(event)
    {
        this._connected = false;
    }

    onMessage(event)
    {
        this._canvas.height = SCREEN_HEIGHT;
        this._canvas.width  = SCREEN_WIDTH;
        var context = this._canvas.getContext('2d');

        var img = new ImageData(
            new Uint8ClampedArray(event.data),
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        );

        this._canvas.height *= 4;
        this._canvas.width  *= 4;

        context.putImageData(img, 0, 0);

        context.scale(4, 4);
        context.drawImage(context.canvas, 0, 0);
    }

    onKeyUp(event)
    {
        this.onKeyPress(EventType.KeyUp, event);
    }

    onKeyDown(event)
    {
        this.onKeyPress(EventType.KeyDown, event);
    }

    onKeyPress(polarity, event)
    {
        if (false === this._connected) { return; }

        if (GAME_KEY_MAP[event.code] !== undefined) {
            if (this._keypress[event.code] === polarity) {
                return;
            }

            this._keypress[event.code] = polarity;
            this.sendEventMessage(polarity, GAME_KEY_MAP[event.code]);
        }
    }

    sendEventMessage(event, data)
    {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "event?" + event + "=" + data, false);
        xhr.send();

        if (204 !== xhr.status) {
            console.log("Unexpected response: " + xhr.status);
        }
    }
}
