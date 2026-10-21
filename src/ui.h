#pragma once

void ui_begin();        // styles + first screen
void ui_boot_button();  // physical BOOT button: pad <-> menu
void ui_poll();         // call from loop(): reacts to saves from the phone editor
