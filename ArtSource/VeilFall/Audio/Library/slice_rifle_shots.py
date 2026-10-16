# Run: python -I slice_rifle_shots.py <Library dir>. Cut single shots out of the two full-auto rifle bursts (mono 48k 16-bit inputs).
import wave, array, sys
lib = sys.argv[1]
def load(p):
    w = wave.open(p); s = array.array('h'); s.frombytes(w.readframes(w.getnframes())); return s, w.getframerate()
def onsets(s, sr, thr=0.35):
    win = int(sr*.005); out = []; last = -1
    for k in range(0, len(s), win):
        pk = max(abs(x) for x in s[k:k+win]) / 32768
        if pk > thr and (last < 0 or k - last > sr*.06): out.append(k); last = k
        if pk > thr: last = k if not out or k-out[-1] < sr*.06 else last
    return out
def write(p, s, sr):
    g = 0.7 * 32767 / max(1, max(abs(x) for x in s)); s = array.array('h', [int(x * g) for x in s])  # peak -3 dBFS
    with wave.open(p, 'wb') as o: o.setnchannels(1); o.setsampwidth(2); o.setframerate(sr); o.writeframes(s.tobytes())
def cut(s, sr, a, b, fade):
    a = max(0, a - int(sr*.004)); seg = array.array('h', s[a:b]); n = len(seg); f = int(sr*fade)
    for i in range(min(f, n)): seg[n-1-i] = int(seg[n-1-i] * i / f)
    for i in range(min(int(sr*.002), n)): seg[i] = int(seg[i] * i / (sr*.002))
    return seg
s, sr = load(lib + '/Weapons/SFX_Gun_Rifle_CombatBurst.wav'); on = onsets(s, sr); print('combat onsets', [round(x/sr,3) for x in on])
write(lib + '/Weapons/SFX_Gun_Rifle_Shot.wav', cut(s, sr, on[-1], len(s), .25), sr)   # last round keeps the full tail
s, sr = load(lib + '/Weapons/SFX_Gun_Rifle_AutoBurst.wav'); on = onsets(s, sr); print('auto onsets', [round(x/sr,3) for x in on])
for i in range(3):
    a = on[i*3]; write(lib + f"/Weapons/SFX_Gun_Rifle_NPC_0{i+1}.wav", cut(s, sr, a, on[i*3+1] - int(sr*.008), .035), sr)
