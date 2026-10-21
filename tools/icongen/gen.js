// Rasterise stratagem SVGs -> LVGL TRUE_COLOR_ALPHA (RGB565 byte-swapped + A8) blob + index header.
// usage: node gen.js <icons_repo_dir> <project_dir>
const sharp = require('sharp');
const fs = require('fs');
const path = require('path');

const [repo, proj] = process.argv.slice(2);
const SIZES = [64, 44];
const PAC = 'Patriotic Administration Center';
const MAP = {
  1: 'Bridge/Orbital Precision Strike', 2: 'Orbital Cannons/Orbital Gatling Barrage', 3: 'Bridge/Orbital Gas Strike',
  4: 'Orbital Cannons/Orbital 120MM HE Barrage', 5: 'Orbital Cannons/Orbital Airburst Strike', 6: 'Bridge/Orbital Smoke Strike',
  7: 'Bridge/Orbital EMS Strike', 8: 'Orbital Cannons/Orbital 380MM HE Barrage', 9: 'Orbital Cannons/Orbital Walking Barrage',
  10: 'Orbital Cannons/Orbital Laser', 11: 'Orbital Cannons/Orbital Napalm Barrage', 12: 'Orbital Cannons/Orbital Railcannon Strike',
  20: 'Hangar/Eagle Strafing Run', 21: 'Hangar/Eagle Airstrike', 22: 'Hangar/Eagle Cluster Bomb', 23: 'Hangar/Eagle Napalm Airstrike',
  24: 'Hangar/Eagle Smoke Strike', 25: 'Hangar/Eagle 110MM Rocket Pods', 26: 'Hangar/Eagle 500KG Bomb', 27: 'Hangar/Eagle Gas Strike',
  40: PAC + '/Machine Gun', 41: PAC + '/Expendable Anti-Tank', 42: PAC + '/Stalwart', 43: 'Engineering Bay/Laser Cannon',
  44: PAC + '/Anti-Materiel Rifle', 45: 'Engineering Bay/Grenade Launcher', 46: PAC + '/Recoilless Rifle', 47: PAC + '/Flamethrower',
  48: PAC + '/Heavy Machine Gun', 49: PAC + '/Autocannon', 50: 'Engineering Bay/Arc Thrower', 51: 'Engineering Bay/Quasar Cannon',
  52: PAC + '/Airburst Rocket Launcher', 53: PAC + '/Commando', 54: PAC + '/Spear', 55: PAC + '/Railgun',
  56: PAC + '/StA-X3 W.A.S.P. Launcher', 57: 'Siege Breakers/CQC-20', 58: 'Control Group/Epoch', 59: 'Exo Experts/Bullet Storm',
  60: 'Chemical Agents/Sterilizer', 61: 'Dust Devils/Speargun', 62: 'Python Commandos/Defoliation Tool',
  63: 'Force of Law/GL-52 De-Escalator', 64: 'Dust Devils/Expendable Napalm', 65: 'Siege Breakers/EAT-411',
  66: 'Python Commandos/Maxigun', 67: 'Siege Breakers/GL-28', 68: 'Entrenched Division/Cremator', 69: 'Dust Devils/Solo Silo',
  70: 'Castellan’s Creed/40-K Meltagun', 71: 'Redacted Regiment/C4 Pack', 72: 'Masters of Ceremony/One True Flag',
  100: 'Engineering Bay/Supply Pack', 101: 'Hangar/Jump Pack', 102: 'Engineering Bay/Ballistic Shield Backpack',
  103: 'Robotics Workshop/Guard Dog', 104: 'Engineering Bay/Guard Dog Rover', 105: 'Engineering Bay/Shield Generator Pack',
  106: 'Urban Legends/Directional Shield', 107: 'Python Commandos/Guard Dog Hot Dog', 108: 'Servants of Freedom/Hellbomb Portable',
  109: 'Force of Law/Guard Dog K-9', 110: 'Borderline Justice/Hover Pack', 111: 'Chemical Agents/Guard Dog Breath',
  112: 'Control Group/Warp Pack',
  130: 'Hangar/Fast Recon Vehicle', 131: 'Hangar/Supply FRV', 132: 'Hangar/Incinerator FRV', 133: 'Robotics Workshop/Patriot Exosuit',
  134: 'Robotics Workshop/Emancipator Exosuit', 135: 'Exo Experts/Breakthrough Exosuit', 136: 'Exo Experts/Lumberer Exosuit',
  137: 'Bridge/TD-110 Maelstrom', 138: 'Siege Breakers/Bastion MK XVI',
  150: 'Robotics Workshop/Machine Gun Sentry', 151: 'Robotics Workshop/Gatling Sentry', 152: 'Robotics Workshop/Mortar Sentry',
  153: 'Robotics Workshop/Autocannon Sentry', 154: 'Robotics Workshop/Rocket Sentry', 155: 'Robotics Workshop/EMS Mortar Sentry',
  156: 'Bridge/Tesla Tower', 157: 'Control Group/Laser Sentry', 158: 'Urban Legends/Flame Sentry',
  159: 'Entrenched Division/Gas Mortar Sentry',
  170: 'Engineering Bay/Anti-Personnel Minefield', 171: 'Engineering Bay/Incendiary Mines', 172: 'Engineering Bay/Anti-Tank Mines',
  173: 'Engineering Bay/Gas Mine', 174: 'Bridge/Shield Generator Relay', 175: 'Bridge/HMG Emplacement',
  176: 'Bridge/Grenadier Battlement', 177: 'Urban Legends/Anti-Tank Emplacement',
  200: 'General Stratagems/Reinforce', 201: 'General Stratagems/Resupply', 202: 'General Stratagems/SOS Beacon',
  203: 'General Stratagems/Hellbomb', 204: 'Hangar/Eagle Rearm', 205: 'General Stratagems/SEAF Artillery',
  206: 'General Stratagems/Super Earth Flag', 208: 'General Stratagems/Upload Data', 209: 'General Stratagems/Seismic Probe',
  210: 'General Stratagems/Hive Breaker Drill', 211: 'General Stratagems/Prospecting Drill',
  212: 'General Stratagems/Dark Fluid Vessel', 213: 'General Stratagems/Tectonic Drill',
  214: 'General Stratagems/Call In Super Destroyer', 217: 'General Stratagems/Cargo Container',
};

async function render(file, size) {
  const { data } = await sharp(file, { density: 300 })
    .resize(size, size, { fit: 'contain', background: { r: 0, g: 0, b: 0, alpha: 0 } })
    .ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  const out = Buffer.alloc(size * size * 3);
  for (let i = 0; i < size * size; i++) {
    const r = data[4 * i], g = data[4 * i + 1], b = data[4 * i + 2], a = data[4 * i + 3];
    const v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    out[3 * i] = v >> 8;      // LV_COLOR_16_SWAP = 1 -> high byte first
    out[3 * i + 1] = v & 0xff;
    out[3 * i + 2] = a;
  }
  return out;
}

(async () => {
  const parts = [];
  const rows = [];
  let off = 0;
  for (const [id, rel] of Object.entries(MAP)) {
    const file = path.join(repo, rel + '.svg');
    if (!fs.existsSync(file)) { console.error('MISSING', id, rel); process.exit(1); }
    const offs = [];
    for (const s of SIZES) {
      const buf = await render(file, s);
      offs.push(off);
      parts.push(buf);
      off += buf.length;
    }
    rows.push(`  {${id}, ${offs.join(', ')}},`);
  }
  fs.mkdirSync(path.join(proj, 'data'), { recursive: true });
  fs.writeFileSync(path.join(proj, 'data', 'icons.bin'), Buffer.concat(parts));
  const hdr = `// Generated by tools/icongen/gen.js - do not edit.
// Icons: github.com/nvigneux/Helldivers-2-Stratagems-icons-svg
#pragma once
#include <stdint.h>
#define ICON_LARGE ${SIZES[0]}
#define ICON_SMALL ${SIZES[1]}
struct IconEntry { uint16_t id; uint32_t offLarge, offSmall; };
static const IconEntry ICON_INDEX[] = {
${rows.join('\n')}
};
#define ICON_COUNT ${rows.length}
`;
  fs.writeFileSync(path.join(proj, 'src', 'icons_index.h'), hdr);
  console.log(`${rows.length} icons, ${off} bytes`);
})();
