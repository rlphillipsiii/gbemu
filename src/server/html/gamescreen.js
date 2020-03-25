
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
        var context = this.canvas.getContext('2d');

        var img = new ImageData(
            new Uint8ClampedArray(event.data),
            this.canvas.width,
            this.canvas.height
        );
        context.putImageData(img, 0, 0);
    }
}
