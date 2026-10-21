# MacroDiver: Helldivers 2 stratagem pad

Firmware for the **Waveshare ESP32-S3-Touch-LCD-1.46** (412×412 round touch screen).
Tap a tile and the pad calls in that stratagem: it holds the stratagem key, sends the arrow code, then releases.

## PS5: read this first

Helldivers 2 **does not accept a keyboard on PS5**, and a PS5 only accepts licensed, authenticated
controllers in PS5 games. No microcontroller can pose as a DualSense by itself. The pad is a
**USB keyboard**, so on PS5 you need one of these:

| Route | How |
|---|---|
| **Keyboard → controller adapter** (recommended) | Use an adapter that takes a USB keyboard and outputs to PS5 using your own DualSense for authentication (XIM Matrix, Cronus Zen, ReaSnow S1, GameSir VX2 or similar). In the adapter, map the pad's **menu key → L1** and its **4 arrow keys → D-pad**. |
| **PS Remote Play / chiaki-ng on a PC** | Plug the pad into the PC running Remote Play (chiaki-ng lets you map keys). Map menu key → L1 and the arrows → D-pad, then set **Speed: Remote Play** on the pad. |
| **PC version** | Works out of the box with the game's default keys: hold **L-Ctrl**, arrows on **W A S D**. |

If an adapter won't recognise the pad, set **Settings → USB serial: Off** and restart. The pad then
shows up as a plain keyboard only, with no extra serial port.

To go **wireless**, plug the dongle (below) into the adapter or PC instead of the pad. The adapter
setup stays the same.

## Wireless dongle (ESP32-S3 Super Mini)

The dongle is a small ESP32-S3 Super Mini that acts as the USB keyboard. The pad sends it each
stratagem over ESP-NOW, a direct radio link between the two boards with no router involved. The
dongle then types the keys with the same timing as the cable. The pad still needs power: a USB
power bank, a charger, or its battery.

**Set up once:**
1. Flash the dongle (see *Building / flashing*), then plug it into the adapter, PS5 or PC. Its LED
   blinks purple, meaning it isn't paired yet.
2. On the pad: *Settings → Output: Wireless dongle*.
3. *Settings → Dongle: tap to pair*. The pad looks for a dongle for 60 s. Pairing happens within
   a second or two, and the row then reads *Dongle: connected*.

From then on, the two find each other at every power-up. The title icon is a Wi-Fi symbol instead of
USB: **green** means the dongle is ready, **red** means it's out of range or not plugged into a
host. The pad's Wi-Fi editor keeps working, because the dongle follows the pad's channel. While
the pad is still joining the home network, the link can drop out for a few seconds.

| Dongle LED | Meaning |
|---|---|
| purple blink | not paired: pair it from the pad |
| blue blink | looking for its pad (pad off, out of range, or Output set to USB) |
| green | ready |
| yellow | linked to the pad, but the USB port isn't a data port (charger or power-only) |
| white | typing |

**Dongle BOOT button:** a short press toggles its USB serial port on or off, then restarts (on by
default; turn it off if an adapter rejects the dongle). Holding it for 3 s makes it forget its pad.
To pair the dongle with your other pad, hold BOOT for 3 s, then pair from that pad. A pad
remembers one dongle; pairing a new one replaces it.

The link isn't encrypted. Only the paired pad and dongle accept each other's packets, but someone
nearby with a radio sniffer could read the stratagems and chat messages you send.

## Using it

- **Pad screen:** tap a tile. It fires on **release**, and only if your finger is still on that tile.
  Sliding off cancels, and swiping left/right changes page without firing anything. The tile gets a
  white ring while its code is being sent. The title shows the build name, the page, and a USB icon
  (green when connected, red when not).
- **Menu:** press the **BOOT** button on the side, or **long-press** the ⚙ icon at the bottom.
  BOOT again goes back to the pad.
- **Builds:** up to 12 saved builds. You can activate, rename, duplicate or delete each one, and
  switch its layout. Everything saves to flash immediately.
- **Edit stratagems:** change the current build in place. Use **Add / remove** to tick or untick
  stratagems by category (up to 24 per build). Tap a row to select it, then use ▲ ▼ to reorder it
  or 🗑 to remove it.
- **Layout:** *Large* is 4 tiles per page (150 px, 30 px gaps). *Dense* is 7 tiles per page
  (116 px, 26 px gaps).
- **Phone editor (Wi-Fi):** edit builds and chat messages from a phone or computer.
  - **Turn it on:** *Settings → Wi-Fi: On*, or open *Menu → Phone editor*. The setting is
    saved and survives restarts.
  - **First time:** with no home network saved, the pad opens its own setup network
    `MacroDiver-XXXX`, whose password is fixed and shown on screen. In *Phone editor*:
    1. Scan the QR to join that network.
    2. Scan the next QR, or wait for the "sign in to network" prompt, to open the page.
    3. In **Pad Wi-Fi**, tap *Scan networks*, pick your home Wi-Fi, enter its password, then
       tap *Connect*. The pad joins your network and remembers it.
    4. Switch the phone back to your home Wi-Fi and open the address shown, e.g.
       `http://192.168.1.42/` or `http://macrodiver.local/`. Bookmark it.
  - **After that:** the editor is always available from your network, at the IP shown under
    *Menu → Phone editor* (updated live). The *Phone editor* screen also shows a QR of that
    address.
  - **If the home network can't be reached for 20 s:** the setup network comes back, so you can
    pick another network. The pad retries the home network every 2 minutes while no phone is on
    the setup network.
  - **On the page:** create, rename, duplicate and delete builds; reorder stratagems; add them by
    category or search (with icons); pick the layout and the active build; edit chat messages.
    Then tap **Save to pad**. If you save while the pad shows a menu, it returns to the pad with
    the new data.
  - **Settings → Network: …** shows the saved home network. Tap twice to forget it.
  - **Security:** anyone on your home network can open the editor (there's no login). Turn
    Wi-Fi off when you don't need it. Custom stratagems are still edited on the pad itself.
- **Chat messages:** canned text you can place on the pad like stratagems (*Edit stratagems → Add /
  remove → Messages*). Tapping one opens chat (Enter by default), types the text and presses Enter.
  Defaults: `gg`, `have to go, see ya`, `see ya`, `bunker`. Up to 12 messages of 40 characters;
  manage them in *Menu → Chat messages*. Set **Settings → Text layout** to match the keyboard layout
  of the receiving PC (e.g. FR AZERTY), otherwise letters come out swapped. On PS5 through a
  controller adapter, typed text generally won't reach the game chat.
- **Custom stratagems:** add new warbond stratagems yourself (a name plus an arrow code) without
  reflashing. LAS-44 Nova and AMW-20 Apex come out on Oct 20, 2026 and have no published codes yet.
- **Settings:**

  | Setting | Options |
  |---|---|
  | Menu key | L-Ctrl / L-Shift / L-Alt / Space / Tab / Caps / Q / E / F |
  | Menu mode | Hold / Tap |
  | Arrows | WASD / arrow keys / IJKL / numpad |
  | Speed | Fast / Normal / Safe / Remote Play |
  | Chat key | Enter / T / Y / None (chat already open) |
  | Text layout | US QWERTY / FR AZERTY / DE QWERTZ / ES / IT |
  | Rotate screen | Off / 180° (display and touch) |
  | Wi-Fi | Off / On (phone editor; persists across restarts) |
  | Network | saved home Wi-Fi; tap twice to forget |
  | Output | USB cable / Wireless dongle |
  | Dongle | link state; tap to pair (60 s), tap again to stop |

  Also: brightness, arrow codes on tiles, USB serial on/off, a test send (Reinforce), restart,
  and factory reset.

Speed presets (wait after opening the menu / key press / gap between keys):

| Preset | Open | Press | Gap |
|---|---|---|---|
| Fast | 50 ms | 25 ms | 25 ms |
| Normal | 80 ms | 40 ms | 40 ms |
| Safe | 120 ms | 60 ms | 60 ms |
| Remote Play | 200 ms | 80 ms | 80 ms |

If inputs get dropped through an adapter, step up to Safe.

## Stratagem list

The full list (Orbital, Eagle, Support Weapons, Backpacks, Vehicles, Sentries, Emplacements,
Mission) is in [src/stratagems.cpp](src/stratagems.cpp). Codes come from helldivers.wiki.gg
(October 2026). Each entry has a fixed ID, so saved builds stay valid. Add new stratagems with new
IDs and never renumber existing ones.

Icons come from [nvigneux/Helldivers-2-Stratagems-icons-svg](https://github.com/nvigneux/Helldivers-2-Stratagems-icons-svg).
To regenerate them after adding entries, update the map in `tools/icongen/gen.js`, then run:

```
cd tools/icongen && npm install && node gen.js <path-to-icons-repo> ../..
```

## Tools and paths (this PC)

Neither `pio` nor a Python with pyserial is on the PATH. Use PlatformIO's own copies
(PowerShell, from the project folder):

```powershell
$PY  = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"        # has pyserial
$PIO = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
$ESPTOOL = "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py"
```

**COM ports:** the running pad is **COM11** (its "USB Serial Device"). When the board is in its
bootloader it shows up as **COM10** ("USB JTAG/serial debug unit"). Port numbers can change on
another PC; check in Device Manager → Ports.

## Backing up builds

Normal firmware updates (`& $PIO run -t upload`) **keep** your builds, settings and messages:
they live in the separate "nvs" flash area, which uploads don't touch. You only need a backup to
survive a **factory reset**, a full flash erase, or a move to a new board.

### Backups present

| File | Contents | Use |
|---|---|---|
| `backups/macrodiver_2026-10-18.txt` | Text backup: settings, builds *Starter*, *terminide1*, *fire*, and the 4 chat messages | Preferred restore |
| `backups/macrodiver_before-wifi.txt` | Same data, taken before the always-on Wi-Fi firmware | Latest text backup |
| `backups/macrodiver_before-web.txt` | Same, taken before the first phone-editor firmware | Older |
| `backups/nvs_2026-10-18.bin` | Raw 20 KB copy of the nvs area (0x9000–0xDFFF), same data | Fallback, exact byte copy |

### Make a new text backup

Requires **Settings → USB serial: On** (the default).

```powershell
& $PY tools\backup.py export COM11 backups\macrodiver_<date>.txt
```

It prints the builds it saved.

### Restore the text backup (preferred)

1. If the board is new or erased, flash the firmware first: `& $PIO run -t upload`. If the upload
   can't connect, hold **BOOT** while plugging in USB, then retry.
2. Make sure the pad shows its pad screen and is on COM11 (USB serial is On by default after a
   reset).
3. Run:
   ```powershell
   & $PY tools\backup.py import COM11 backups\macrodiver_2026-10-18.txt
   ```
   It prints `restored: OK imported, restarting`. The pad **replaces** all builds, custom
   stratagems, messages and settings with the file's content, then reboots.
4. Check: `& $PY tools\backup.py export COM11 check.txt` should match the backup file.

The file is plain text and can be edited by hand before importing:

```
MACRODIVER-BACKUP 1
S menuKey=0 menuMode=0 dirSet=0 speed=1 brightness=80 showCodes=1 usbSerial=1 activeBuild=2 rotate180=1 chatKey=0 textLayout=0
B <layout 0=Large 1=Dense>|<name>|<id>,<id>,...      one line per build, in order
C <id>|<name>|<code U/D/L/R>                          custom stratagem (ids 1000+)
M <id>|<text>                                         chat message (ids 2000+)
END
```

The text backup includes the Wi-Fi on/off setting but **not** the home Wi-Fi password: after a
restore on an erased board, set the network again from the phone editor. The raw `.bin` copy does
include it.

Stratagem IDs are listed in `src/stratagems.cpp` (e.g. 200 = Reinforce, 51 = Quasar Cannon). An
import with no builds is refused and nothing is changed.

### Restore the raw copy (fallback)

This is only valid with a firmware that has the **same data layout** as when the copy was made
(October 2026 firmware). With a newer firmware, use the text backup instead.

1. Put the board in its bootloader: close anything using COM11, then
   `& $PY -c "import serial,time; s=serial.Serial('COM11',1200); time.sleep(0.2); s.close()"`.
   Or hold **BOOT** while plugging in USB. It now appears as COM10.
2. Write the copy back and reboot:
   ```powershell
   & $PY $ESPTOOL --chip esp32s3 --port COM10 write_flash 0x9000 backups\nvs_2026-10-18.bin
   ```
3. If it doesn't restart by itself, unplug and replug it.

To make a new raw copy: step 1, then
`& $PY $ESPTOOL --chip esp32s3 --port COM10 --after no_reset read_flash 0x9000 0x5000 backups\nvs_<date>.bin`,
then replug.

## Building / flashing

PlatformIO (pioarduino 53.03.11, Arduino-ESP32 3.1.1, LVGL 8.3.10):

```powershell
& $PIO run -t upload
```

The upload resets the board into its bootloader over USB automatically (the 1200-baud trick on
COM11, then it flashes on COM10). The final "Cannot configure port" message is harmless: the port
disappears when the board restarts; look for `Hash of data verified`. If the board doesn't respond
(for example with USB serial turned off), hold **BOOT** while plugging it in, then upload.
Build output goes to `C:/Users/odu/.pio-build/macrodiver`, because the project folder is on
Google Drive.

**Debug commands** (USB serial, 115200 baud): `status`, `shot`, `tap x y`, `hold x y ms`,
`swipe x1 y1 x2 y2`, `boot` (= BOOT button), `say <text>` (types a chat message on the focused
window), `export`, import lines (`MACRODIVER-BACKUP` … `END`), and `webstate` / `websave <json>`
(the phone editor's API without Wi-Fi; `websave` replaces builds and messages).
`tools/pad.py COM11 status "tap x y" boot shot:file.png` sends them and saves screenshots as PNG.
Careful: taps on pad tiles and `say` send real keystrokes to whichever window has focus on the
connected PC.

Display, touch and IO-expander drivers in `src/board/` are from Waveshare's demo code.

### Project layout

| Folder | What |
|---|---|
| `src/`, `include/`, `platformio.ini` | Pad firmware (Waveshare board) |
| `dongle/` | Dongle firmware (ESP32-S3 Super Mini), its own PlatformIO project |
| `lib/macrolink/` | Shared by both: radio packets and the keystroke sequencer (`macro_run`) |
| `tools/` | Backup, debug and icon tools |

Change the keystroke timing or the packet format in `lib/macrolink` only, then reflash **both**
boards. The packets carry a version number, so a mismatched pair ignores each other instead of
misbehaving.

### Flashing the dongle

```powershell
cd dongle
& $PIO run -t upload
```

The first time, hold the Super Mini's **BOOT** button while plugging it in, so it starts in its
bootloader. After that, uploads reset it automatically, as long as its USB serial is on. Its
`status` serial command prints its pairing, channel and USB state. Build output goes to
`C:/Users/odu/.pio-build/macrodiver-dongle`. `-DWIFI_COUNTRY` in `dongle/platformio.ini` (FR)
enables Wi-Fi channels 12–13. `-DLED_PIN` is the RGB LED, GPIO48 on most Super Minis.

The pad's `status` debug command also reports `out` (0 = USB, 1 = wireless), `paired`, `dongle`
(heard recently), `pairing` (seconds left) and `ch` (radio channel).
