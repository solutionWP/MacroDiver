#pragma once
#include <stdint.h>

// Wi-Fi + web editor (builds, messages, home Wi-Fi setup).
// When enabled (Settings -> Wi-Fi, persistent):
//  - with a saved home network, the pad joins it and serves the editor at its IP
//    and http://macrodiver.local/
//  - with none, or if joining fails, it opens its own setup network
//    (MacroDiver-XXXX, fixed password) where the phone can enter the home Wi-Fi.
enum NetState : uint8_t { NET_OFF, NET_CONNECTING, NET_ONLINE, NET_SETUP };

void web_begin();                  // at boot: starts if Settings -> Wi-Fi is on
void web_set_enabled(bool on);     // saves the setting
bool web_enabled();
void web_loop();                   // call from loop()
NetState web_state();

const char *web_ip();              // home-network IP when online, else setup IP, else ""
const char *web_url();             // http://<ip>/
const char *web_home_ssid();       // saved home network, "" if none
void web_forget_home();            // clears saved home network -> setup mode
bool web_ap_on();
const char *web_ap_ssid();
const char *web_ap_pass();
int web_ap_clients();              // phones on the setup network
uint32_t web_save_count();         // increments on every save from the page

// Shared by the HTTP API and the serial debug commands (webstate / websave)
class String;
String web_state_json();
const char *web_apply_json(const char *json);  // nullptr = saved, else error text
