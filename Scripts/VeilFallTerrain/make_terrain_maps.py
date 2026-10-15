"""Generate the Landscape heightmap and paint masks for L_VF_IntroProperty (plain Python 3, no packages).

Output (ArtSource/VeilFall/Terrain):
  HM_VF_IntroProperty_1009.png   16-bit greyscale heightmap, 1009 x 1009 vertices
  WM_VF_<Layer>_1009.png          8-bit masks for Pasture, Gravel, Dirt, Asphalt (Grass is the base layer)

Import settings (Landscape mode > Import from File), matching the greybox coordinates:
  Location  X=-39400  Y=-50400  Z=0      Scale  X=100  Y=100  Z=100
  -> 1 vertex per metre over 1008 m x 1008 m centred on the property; heightmap value 32768 is Z=0.
The property core (house, shop, apron, driveway, county road, gate) is kept at exactly Z=0 so every greybox
building, marker and nav point stays where it is. Hills from the greybox mounds are reproduced; rolling
farmland rises gently outside the fences.
"""
import math, struct, zlib, random
from pathlib import Path

N = 1009                       # vertices per side
ORIGIN_X, ORIGIN_Y = -39400.0, -50400.0
STEP = 100.0                   # cm per vertex
OUT = Path(__file__).resolve().parents[2] / 'ArtSource' / 'VeilFall' / 'Terrain'

def smooth(e0, e1, x):
    t = min(1.0, max(0.0, (x - e0) / (e1 - e0))); return t * t * (3 - 2 * t)

def box_dist(x, y, x0, x1, y0, y1):
    dx = max(x0 - x, 0.0, x - x1); dy = max(y0 - y, 0.0, y - y1); return math.hypot(dx, dy)

# Zones that must stay at exactly Z=0 (cm, world space): name, x0, x1, y0, y1, blend distance
FLAT = [
    ('buildings+yard', -3200, 2800, -3800, 3000, 1500),
    ('driveway', 800, 25600, 300, 1900, 900),
    ('county road', 25600, 26900, -60000, 60000, 800),
    ('gate', 24500, 26200, 0, 2200, 600),
]
# Greybox mounds reproduced as smooth hills: cx, cy, half-width x, half-width y, height, falloff
HILLS = [(14000, 9500, 2000, 1250, 520, 3200), (9500, -9500, 2500, 1250, 300, 1900),
         (21500, 7500, 1250, 1250, 380, 2400), (-2400, 700, 450, 900, 300, 900)]

rnd = random.Random(7)
WAVES = [(rnd.uniform(0, 6.28), rnd.uniform(0, 6.28), rnd.uniform(1 / 26000, 1 / 9000), rnd.uniform(1 / 26000, 1 / 9000), rnd.uniform(.3, 1)) for _ in range(6)]

def height(x, y):
    h = 0.0
    for cx, cy, hx, hy, top, fall in HILLS:
        d = box_dist(x, y, cx - hx, cx + hx, cy - hy, cy + hy)
        h = max(h, top * (1 - smooth(0, fall, d)))
    # Gentle field undulation inside the fences, real rolling hills outside them.
    outside = box_dist(x, y, -6000, 25000, -12000, 12000)
    roll = sum(a * math.sin(px + x * fx) * math.cos(py + y * fy) for px, py, fx, fy, a in WAVES) / 3.0
    # Inside the fences only rise (|roll|), so fence posts and trees placed at Z=0 never float.
    h += abs(roll) * 60 * (1 - smooth(0, 3000, outside)) + roll * 2400 * smooth(0, 20000, outside)
    h += 1800 * smooth(15000, 45000, outside)          # land climbs toward the horizon
    if x < -6000: h += 500 * smooth(0, 6000, -6000 - x)  # treeline ridge behind the house
    # Keep the playable core flat at Z=0.
    keep = 1.0
    for _, x0, x1, y0, y1, blend in FLAT:
        keep = min(keep, smooth(0, blend, box_dist(x, y, x0, x1, y0, y1)))
    # Earth-sheltered vault berm is allowed right up to the vault's back wall.
    if -3300 < x < -1500 and -400 < y < 1800: keep = max(keep, smooth(-1500, -2000, x))
    return h * keep

MASKS = {
    'Gravel': lambda x, y: 1 - smooth(300, 450, box_dist(x, y, 800, 25000, 775, 1425)),
    'Asphalt': lambda x, y: 1 - smooth(450, 550, abs(x - 26200)),
    'Dirt': lambda x, y: max(1 - smooth(150, 300, box_dist(x, y, 11500, 17500, 2300, 2700)), 1 - smooth(200, 600, box_dist(x, y, 800, 2500, -2400, -200))),
    'Pasture': lambda x, y: max(1 - smooth(0, 800, box_dist(x, y, 3000, 21000, -11500, -3500)), 1 - smooth(0, 800, box_dist(x, y, 4000, 20000, 5000, 11000))),
}

def write_png(path, rows, bits):
    raw = b''.join(b'\x00' + r for r in rows)
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    ihdr = struct.pack('>IIBBBBB', N, N, bits, 0, 0, 0, 0)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    hrows, mrows = [], {k: [] for k in MASKS}
    lo, hi = 1e9, -1e9
    for j in range(N):
        y = ORIGIN_Y + j * STEP
        hr = bytearray(); mr = {k: bytearray() for k in MASKS}
        for i in range(N):
            x = ORIGIN_X + i * STEP
            h = height(x, y); lo = min(lo, h); hi = max(hi, h)
            v = int(round(32768 + h * 128 / 100.0))     # Z scale 100: 1 unit = 100/128 cm
            hr += struct.pack('>H', max(0, min(65535, v)))
            g = MASKS['Gravel'](x, y); a = MASKS['Asphalt'](x, y)
            d = MASKS['Dirt'](x, y) * (1 - g); p = MASKS['Pasture'](x, y) * (1 - max(g, a, d))
            for k, val in (('Gravel', g), ('Asphalt', a), ('Dirt', d), ('Pasture', p)): mr[k].append(int(255 * max(0.0, min(1.0, val))))
        hrows.append(bytes(hr))
        for k in MASKS: mrows[k].append(bytes(mr[k]))
    write_png(OUT / 'HM_VF_IntroProperty_1009.png', hrows, 16)
    for k in MASKS: write_png(OUT / ('WM_VF_%s_1009.png' % k), mrows[k], 8)
    print('terrain maps written to %s  height range %.1f m .. %.1f m' % (OUT, lo / 100, hi / 100))

if __name__ == '__main__':
    main()
