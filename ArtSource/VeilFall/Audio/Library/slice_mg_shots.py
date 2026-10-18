# Run: python -I slice_mg_shots.py <Library dir>. Cut single rounds out of Matthew's machine gun burst (SFX_Gun_MachineGun_Burst, mono 48k 16-bit),
# the same way slice_rifle_shots.py does for the rifle: the last round keeps its full tail (player shot), three mid-burst rounds become NPC variants.
import wave, array, sys
lib = sys.argv[1]
w = wave.open(lib + '/Weapons/SFX_Gun_MachineGun_Burst.wav'); sr = w.getframerate(); s = array.array('h'); s.frombytes(w.readframes(w.getnframes()))
win = int(sr * .005); env = [max(abs(x) for x in s[k:k + win]) / 32768 for k in range(0, len(s), win)]
on = []
for i, e in enumerate(env):  # rising edge into a peak, at least 50 ms after the previous round
    if e > .95 and (i == 0 or env[i - 1] <= .95) and (not on or (i * win - on[-1]) > sr * .05): on.append(i * win)
print('mg onsets', len(on), [round(x / sr, 3) for x in on])
def write(p, seg):
    g = 0.7 * 32767 / max(1, max(abs(x) for x in seg)); seg = array.array('h', [int(x * g) for x in seg])  # peak -3 dBFS
    with wave.open(p, 'wb') as o: o.setnchannels(1); o.setsampwidth(2); o.setframerate(sr); o.writeframes(seg.tobytes())
def cut(a, b, fade):
    a = max(0, a - int(sr * .006)); seg = array.array('h', s[a:b]); n = len(seg); f = int(sr * fade)
    for i in range(min(f, n)): seg[n - 1 - i] = int(seg[n - 1 - i] * i / f)
    for i in range(min(int(sr * .002), n)): seg[i] = int(seg[i] * i / (sr * .002))
    return seg
write(lib + '/Weapons/SFX_Gun_MachineGun_Shot.wav', cut(on[-1], len(s), .25))
for i, k in enumerate((2, 6, 10)):
    write(lib + '/Weapons/SFX_Gun_MachineGun_NPC_0%d.wav' % (i + 1), cut(on[k], on[k + 1] - int(sr * .006), .03))
