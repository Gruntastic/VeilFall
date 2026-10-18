"""Convert a WAV to the library format (48 kHz, 16-bit, mono) with only the standard library.
Usage: python convert_wav.py <source.wav> <Library/Category/SFX_Name.wav>
Handles 16/24/32-bit PCM, any channel count; integer-ratio downsampling (e.g. 96 kHz -> 48 kHz) averages each block.
"""
import sys, wave, struct

src, dst = sys.argv[1], sys.argv[2]
with wave.open(src, 'rb') as w:
    ch, width, rate, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
    raw = w.readframes(n)
assert rate % 48000 == 0 or rate == 44100, 'unsupported rate %d' % rate
def sample(i):
    b = raw[i * width:(i + 1) * width]
    if width == 3: return int.from_bytes(b, 'little', signed=True) / 8388608.0
    if width == 2: return struct.unpack('<h', b)[0] / 32768.0
    if width == 4: return struct.unpack('<i', b)[0] / 2147483648.0
    raise ValueError(width)
mono = [sum(sample(f * ch + c) for c in range(ch)) / ch for f in range(n)]
if rate == 44100:  # linear-interpolate up to 48 kHz
    out_n = int(n * 48000 / 44100); out = []
    for i in range(out_n):
        x = i * 44100 / 48000; j = int(x); t = x - j
        out.append(mono[j] * (1 - t) + mono[min(j + 1, n - 1)] * t)
else:
    k = rate // 48000
    out = [sum(mono[i:i + k]) / k for i in range(0, n - n % k, k)]
with wave.open(dst, 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)
    w.writeframes(b''.join(struct.pack('<h', max(-32768, min(32767, int(round(v * 32767))))) for v in out))
print('converted %s: %d ch %d-bit %d Hz -> mono 16-bit 48 kHz, %.2f s, peak %.2f' % (src, ch, width * 8, rate, len(out) / 48000, max(abs(v) for v in out)))
