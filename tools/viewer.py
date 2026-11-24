# Shows the koi pond from the board, scaled up, in your web browser.
#
#   1. Upload the firmware as usual.
#   2. Close the PlatformIO serial monitor (only one program can use COM3 at a time).
#   3. Run:   ~/.platformio/penv/Scripts/python.exe tools/viewer.py
#      (or pass a port:   ... tools/viewer.py COM5)
#   4. Open   http://localhost:8765   in your browser.
#
# Press S (or the button) in the browser to save a PNG screenshot.
# The board's normal text output (frame times etc.) is printed here.
#
# How it works: this script keeps asking the board for frames ('F'), and
# serves the latest one to the browser, which draws it with crisp pixels.

import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import serial  # pyserial, comes with PlatformIO

PORT_NAME = sys.argv[1] if len(sys.argv) > 1 else "COM3"
HTTP_PORT = 8765
MAGIC = b"KOIF"

latest_frame = None            # bytes: width (2) + height (2) + pixels
latest_frame_lock = threading.Lock()


# =============================================================================
# Serial: ask the board for frames
# =============================================================================

def read_exact(ser, count):
    data = bytearray()
    while len(data) < count:
        chunk = ser.read(count - len(data))
        if not chunk:
            raise TimeoutError("board stopped sending mid-frame")
        data += chunk
    return bytes(data)


def read_frame(ser, text_buffer):
    # Everything before "KOIF" is ordinary text (Serial.printf), so print it.
    window = bytearray()
    while True:
        byte = ser.read(1)
        if not byte:
            raise TimeoutError("no frame (is the new firmware uploaded?)")
        window += byte
        if window.endswith(MAGIC):
            text_buffer += window[:-len(MAGIC)]
            break
        if len(window) > 4096:                        # lots of text, flush some
            text_buffer += window[:-len(MAGIC)]
            del window[:-len(MAGIC)]
        flush_text(text_buffer)

    flush_text(text_buffer)
    size = read_exact(ser, 4)
    width = int.from_bytes(size[0:2], "little")
    height = int.from_bytes(size[2:4], "little")
    pixels = read_exact(ser, width * height * 2)
    return size + pixels


def flush_text(text_buffer):
    while b"\n" in text_buffer:
        line, _, rest = bytes(text_buffer).partition(b"\n")
        print("board:", line.decode("utf-8", "replace").rstrip())
        text_buffer[:] = rest


def serial_loop():
    global latest_frame
    while True:
        try:
            ser = serial.Serial()
            ser.port = PORT_NAME
            ser.timeout = 1.0
            ser.dtr = False   # don't poke the reset lines when opening
            ser.rts = False
            ser.open()
            print("connected to", PORT_NAME)
            text_buffer = bytearray()
            frames = 0
            last_report = time.time()
            while True:
                ser.write(b"F")
                frame = read_frame(ser, text_buffer)
                with latest_frame_lock:
                    latest_frame = frame
                frames += 1
                if time.time() - last_report >= 5:
                    print("viewer: %.1f frames/s" % (frames / (time.time() - last_report)))
                    frames, last_report = 0, time.time()
        except (serial.SerialException, TimeoutError, OSError) as error:
            print("serial:", error, "- retrying in 1s")
            try:
                ser.close()
            except Exception:
                pass
            time.sleep(1)


# =============================================================================
# Browser: serve the page and the latest frame
# =============================================================================

PAGE = b"""<!doctype html>
<html><head><meta charset="utf-8"><title>Koi pond viewer</title>
<style>
  html, body { margin: 0; height: 100%; background: #111; color: #aaa; font: 13px sans-serif; }
  body { display: flex; flex-direction: column; align-items: center; justify-content: center; }
  canvas { image-rendering: pixelated; image-rendering: crisp-edges; }
  #bar { padding: 8px; }
  button { font: inherit; }
</style></head>
<body>
  <canvas id="view"></canvas>
  <div id="bar"><button id="shot">Save PNG (S)</button> <span id="info">waiting for frames...</span></div>
<script>
const view = document.getElementById("view");
const info = document.getElementById("info");
const small = document.createElement("canvas");   // the frame at its real size
const smallCtx = small.getContext("2d");
let image = null;

function fitToWindow() {
  if (!small.width) return;
  const scale = Math.max(1, Math.floor(Math.min(
      (innerWidth - 20) / small.width, (innerHeight - 60) / small.height)));
  view.width = small.width * scale;      // whole-number zoom keeps every pixel square
  view.height = small.height * scale;
  const ctx = view.getContext("2d");
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(small, 0, 0, view.width, view.height);
  info.textContent = small.width + " x " + small.height + " at " + scale + "x";
}

function showFrame(bytes) {
  const width = bytes[0] | (bytes[1] << 8);
  const height = bytes[2] | (bytes[3] << 8);
  if (small.width !== width || small.height !== height) {
    small.width = width; small.height = height;
    image = smallCtx.createImageData(width, height);
  }
  const out = image.data;
  for (let i = 0, p = 4; i < width * height; i++, p += 2) {
    const v = (bytes[p] << 8) | bytes[p + 1];    // sprite stores high byte first
    out[i * 4]     = ((v >> 11) & 31) * 255 / 31;
    out[i * 4 + 1] = ((v >> 5) & 63) * 255 / 63;
    out[i * 4 + 2] = (v & 31) * 255 / 31;
    out[i * 4 + 3] = 255;
  }
  smallCtx.putImageData(image, 0, 0);
  fitToWindow();
}

async function poll() {
  try {
    const response = await fetch("/frame", { cache: "no-store" });
    if (response.status === 200) showFrame(new Uint8Array(await response.arrayBuffer()));
  } catch (e) { info.textContent = "viewer.py not running?"; }
  setTimeout(poll, 30);
}

function screenshot() {
  const link = document.createElement("a");
  link.download = "koi-" + new Date().toISOString().replace(/[:.]/g, "-") + ".png";
  link.href = view.toDataURL("image/png");
  link.click();
}

document.getElementById("shot").onclick = screenshot;
addEventListener("keydown", e => { if (e.key === "s" || e.key === "S") screenshot(); });
addEventListener("resize", fitToWindow);
poll();
</script></body></html>
"""


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.reply(200, "text/html", PAGE)
        elif self.path == "/frame":
            with latest_frame_lock:
                frame = latest_frame
            if frame is None:
                self.reply(204, "text/plain", b"")
            else:
                self.reply(200, "application/octet-stream", frame)
        else:
            self.reply(404, "text/plain", b"not found")

    def reply(self, status, content_type, body):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):   # keep the console for board output
        pass


if __name__ == "__main__":
    threading.Thread(target=serial_loop, daemon=True).start()
    server = ThreadingHTTPServer(("127.0.0.1", HTTP_PORT), Handler)
    print("open http://localhost:%d in your browser (Ctrl+C to stop)" % HTTP_PORT)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
