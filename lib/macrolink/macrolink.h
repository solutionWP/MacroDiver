#pragma once
// Shared by the pad and the wireless dongle (dongle/):
//  - MacroJob: one stratagem or chat message, with every key already resolved,
//    so whoever types it needs no settings of its own.
//  - macro_run(): types a job on a USB keyboard with the job's timing.
//  - The ESP-NOW packets between pad and dongle.
//
// Link: the dongle hops Wi-Fi channels broadcasting PING until its pad answers
// PONG, then stays on that channel and sends PING every second as a heartbeat.
// The pad sends JOB; the dongle types it and reports progress with STATUS.
// Pairing: a pad in pairing mode answers an unpaired dongle's PING and keeps
// its MAC; the dongle keeps the MAC of the first pad whose PONG says pairing.
// Otherwise each side only talks to the MAC it remembers.
#include <stdint.h>
#include <stddef.h>

#define MACRO_DATA_LEN 42  // stratagem code or chat text (pad: MSG_LEN 40)

enum MacroKind : uint8_t { MJ_STRAT = 0, MJ_TEXT = 1 };

struct __attribute__((packed)) MacroJob {
  uint8_t kind;      // MacroKind
  uint8_t menuKey;   // HID usage of the stratagem menu key
  uint8_t menuHold;  // 1 = hold it during the code, 0 = tap it first
  uint8_t chatKey;   // HID usage that opens chat, 0 = chat already open
  uint8_t layout;    // index into MACRO_LAYOUTS, for typed text
  uint8_t up, down, left, right;  // HID usages for U/D/L/R
  uint16_t openMs, pressMs, gapMs;
  char data[MACRO_DATA_LEN + 1];  // U/D/L/R code, or text
};

// Text keyboard layouts. Must match the layout set on the receiving host.
struct MacroLayout { const char *label; const uint8_t *map; };
extern const MacroLayout MACRO_LAYOUTS[];
extern const size_t MACRO_LAYOUT_COUNT;

class USBHIDKeyboard;
// Blocking: call from a task of its own. onStep(i) is called as character i starts.
void macro_run(USBHIDKeyboard &kb, const MacroJob &job, void (*onStep)(int));

// ---------------------------------------------------------------- link

#define LINK_MAGIC       0x4D44  // "MD"
#define LINK_VERSION     2
#define LINK_PING_MS     1000    // dongle heartbeat once linked
#define LINK_TIMEOUT_MS  3500    // no PING/PONG for this long = link lost
#define LINK_PAIR_MS     60000   // pad pairing window
#define LINK_IDLE_CHANNEL 1      // pad channel when its Wi-Fi editor is off

enum LinkType : uint8_t { LINK_PING = 1, LINK_PONG = 2, LINK_JOB = 3, LINK_STATUS = 4 };

struct __attribute__((packed)) LinkHeader {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
};

struct __attribute__((packed)) LinkPing {  // dongle -> pad (broadcast while searching)
  LinkHeader h;
  uint8_t usb;   // dongle is mounted by a USB host
  uint8_t busy;  // dongle is typing
  uint8_t unpaired;  // dongle has no pad yet and wants one in pairing mode
};

struct __attribute__((packed)) LinkPong {  // pad -> dongle
  LinkHeader h;
  uint8_t pairing;  // pad accepts a new dongle
  uint8_t channel;  // pad's Wi-Fi channel: neighbouring channels leak at short range, so the
                    // channel a PONG was heard on isn't always the pad's
};

struct __attribute__((packed)) LinkJobMsg {  // pad -> dongle
  LinkHeader h;
  uint16_t seq;  // dongle ignores a repeat of the last seq
  MacroJob job;
};

struct __attribute__((packed)) LinkStatus {  // dongle -> pad
  LinkHeader h;
  uint16_t seq;
  int8_t step;   // character being typed, -1 when done
  uint8_t done;
  uint8_t usb;
};

inline void link_header(LinkHeader &h, LinkType t) {
  h.magic = LINK_MAGIC;
  h.version = LINK_VERSION;
  h.type = t;
}

// Type of a received packet, or 0 if it is not a valid one of ours.
inline uint8_t link_type(const uint8_t *data, int len) {
  if (len < (int)sizeof(LinkHeader)) return 0;
  const LinkHeader *h = (const LinkHeader *)data;
  if (h->magic != LINK_MAGIC || h->version != LINK_VERSION) return 0;
  static const uint8_t SIZES[] = {0, sizeof(LinkPing), sizeof(LinkPong), sizeof(LinkJobMsg), sizeof(LinkStatus)};
  if (h->type == 0 || h->type >= sizeof(SIZES) || len != SIZES[h->type]) return 0;
  return h->type;
}
