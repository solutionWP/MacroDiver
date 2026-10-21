"""Drive MacroDiver over its USB serial port.
usage: pad.py PORT cmd [cmd ...]   e.g. pad.py COM11 status "tap 100 100" shot:home.png
A 'shot:<file>' command saves a PNG screenshot (circle-masked)."""
import serial, sys, time, zlib, struct


def png_write(path, w, h, rgb):
    raw = b''.join(b'\x00' + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def read_line(s, timeout=5):
    end = time.time() + timeout
    buf = b''
    while time.time() < end:
        c = s.read(1)
        if not c:
            continue
        if c == b'\n':
            return buf.decode('utf-8', 'replace')
        buf += c
    return buf.decode('utf-8', 'replace')


def shot(s, path):
    s.write(b'shot\n')
    while True:
        line = read_line(s, 10)
        if line.startswith('SHOT'):
            break
        if not line:
            raise RuntimeError('no SHOT header')
    _, w, h, n = line.split()
    w, h, n = int(w), int(h), int(n)
    data = b''
    end = time.time() + 20
    while len(data) < n and time.time() < end:
        data += s.read(n - len(data))
    print(line, "received", len(data))
    if len(data) < n: raise RuntimeError("short")
    read_line(s); read_line(s)
    rgb = bytearray(w * h * 3)
    cx, cy, r2 = w / 2, h / 2, (w / 2) ** 2
    for i in range(w * h):
        v = (data[2 * i] << 8) | data[2 * i + 1]  # big-endian (LV_COLOR_16_SWAP)
        x, y = i % w, i // w
        if (x - cx) ** 2 + (y - cy) ** 2 > r2:
            rgb[3 * i:3 * i + 3] = b'\x40\x40\x40'
            continue
        rgb[3 * i] = ((v >> 11) & 0x1f) * 255 // 31
        rgb[3 * i + 1] = ((v >> 5) & 0x3f) * 255 // 63
        rgb[3 * i + 2] = (v & 0x1f) * 255 // 31
    png_write(path, w, h, bytes(rgb))
    print('saved', path)


def main():
    s = serial.Serial(sys.argv[1], 115200, timeout=0.3)
    time.sleep(0.3)
    s.reset_input_buffer()
    for cmd in sys.argv[2:]:
        if cmd.startswith('shot:'):
            shot(s, cmd[5:])
        elif cmd.startswith('sleep:'):
            time.sleep(float(cmd[6:]))
        else:
            s.write((cmd + '\n').encode())
            print(cmd, '->', read_line(s, 6))
    s.close()


main()
