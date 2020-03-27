"use strict";

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

        this._init();

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

    _init()
    {
        this._busy  = false;
        this._queue = [];

        var addr = self.location.host.split(":")[0];

        this._socket = new WebSocket("ws://" + addr + ":1148/");
        this._socket.binaryType = "arraybuffer";

        this._socket.onopen    = (e) => { this.onOpen(e);    };
        this._socket.onclose   = (e) => { this.onClose(e);   };
        this._socket.onerror   = (e) => { this.onError(e);   };
        this._socket.onmessage = (e) => { this.onMessage(e); };
    }

    onOpen(event)
    {
        this._connected = true;
    }

    onClose(event)
    {
        this._connected = false;
    }

    onError(event)
    {
        console.log("connection failed");

        this._connected = false;

        window.setTimeout(() => { this._init(); }, 250);
    }

    onMessage(event)
    {
        this._socket.send("r");

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
        if (false === this._connected) { return null; }

        if (this._busy) {
            this._queue.push({ "event" : event, "data"  : data, });
        } else {
            this._busy = true;
            return this._send(event, data);
        }
        return null;
    }

    _send(event, data)
    {
        var xhr = new XMLHttpRequest();
        xhr.addEventListener('loadend', (evt) => {
            if (204 !== xhr.status) {
                console.log("Unexpected response: " + xhr.status);
            }

            if (0 === this._queue.length) {
                this._busy = false;
            } else {
                var next = this._queue.shift();
                this._send(next.event, next.data);
            }
        });

        xhr.open("GET", "event?" + event + "=" + data);
        xhr.send();

        return xhr;
    }
}
