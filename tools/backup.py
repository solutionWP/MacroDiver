"""Back up / restore MacroDiver builds, settings, custom stratagems and chat messages.

usage:
  python backup.py export COM11 backups/macrodiver.txt
  python backup.py import COM11 backups/macrodiver.txt   (replaces data on the pad, then it restarts)

Needs "USB serial: On" on the pad. The file is plain text and can be edited by hand.
"""
import sys
import time
import serial


def read_line(s, timeout=5):
    end = time.time() + timeout
    buf = b''
    while time.time() < end:
        c = s.read(1)
        if not c:
            continue
        if c == b'\n':
            return buf.decode('utf-8', 'replace').rstrip('\r')
        buf += c
    raise TimeoutError('no reply from pad')


def export(port, path):
    with serial.Serial(port, 115200, timeout=0.3) as s:
        time.sleep(0.3)
        s.reset_input_buffer()
        s.write(b'export\n')
        lines = []
        while True:
            line = read_line(s)
            if not lines and not line.startswith('MACRODIVER-BACKUP'):
                continue  # skip stray log output
            lines.append(line)
            if line == 'END':
                break
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    builds = [l for l in lines if l.startswith('B ')]
    print(f'saved {path}: {len(builds)} builds, '
          f'{sum(l.startswith("C ") for l in lines)} custom, {sum(l.startswith("M ") for l in lines)} messages')
    for b in builds:
        layout, name, ids = b[2:].split('|')
        print(f'  - {name} ({len(ids.split(",")) if ids else 0} items)')


def restore(port, path):
    lines = [l.rstrip('\r\n') for l in open(path, encoding='utf-8') if l.strip()]
    if not lines or not lines[0].startswith('MACRODIVER-BACKUP') or lines[-1] != 'END':
        sys.exit('not a MacroDiver backup file')
    with serial.Serial(port, 115200, timeout=0.3) as s:
        time.sleep(0.3)
        s.reset_input_buffer()
        for line in lines:
            s.write((line + '\n').encode('utf-8'))
            reply = read_line(s)
            while not (reply.startswith('OK') or reply.startswith('ERR')):
                reply = read_line(s)
            if reply.startswith('ERR'):
                sys.exit(f'pad refused: {reply}')
        print('restored:', reply)


if __name__ == '__main__':
    if len(sys.argv) != 4 or sys.argv[1] not in ('export', 'import'):
        sys.exit(__doc__)
    (export if sys.argv[1] == 'export' else restore)(sys.argv[2], sys.argv[3])
