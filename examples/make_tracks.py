"""Made-up tracks to try the examples on, and to hear Automix with (no one's music): dance loops with a kick, a bass line,
hats and a pad; an intro, a drop where the kick and the bass come in together, a break without the kick, and an outro.

    python examples/make_tracks.py <out folder>

writes, 2 minutes each, 44.1 kHz stereo:
  a-124bpm.wav   124 BPM, A minor
  b-124bpm.wav   124 BPM, C major (A minor's relative: the same place on the Camelot wheel), its drop after 16 bars
  c-96bpm.wav    96 BPM, G minor: too far from 124 to join on the beat
"""
import math, os, random, struct, sys, wave

RATE = 44100


def track(path, bpm, root, minor, seed, drop_bar, seconds=120):
    rnd = random.Random(seed)
    beat = 60.0 / bpm
    third = 1.189 if minor else 1.26          # a minor or a major third
    notes = [root, root * 0.75 * 2, root * third, root * 1.5 / 2]   # a four-bar bass line
    end_bar = int((seconds - 8) / (4 * beat))  # the outro: the kick and the bass leave 8 s before the end
    out = bytearray()
    for i in range(int(seconds * RATE)):
        t = i / RATE
        k = int(t / beat)
        bar, u = k // 4, t - k * beat
        body = drop_bar <= bar < end_bar
        brk = body and ((bar - drop_bar) // 8) % 4 == 3   # every fourth phrase after the drop: a break without the kick
        edge = min(1.0, t / 2, (seconds - t) / 6)
        v = 0.0
        if body and not brk:   # the kick
            v += math.sin(2 * math.pi * (48 + 90 * math.exp(-u * 32)) * u) * math.exp(-u * 8) * (1.0 if k % 4 == 0 else 0.62)
        if body:               # the bass, with the kick from the drop on
            f = notes[bar % 4]
            v += math.sin(2 * math.pi * f * t) * 0.24 * (0.25 if brk else 1) * min(1.0, u * 40) * math.exp(-u * 2)
        h = (t + beat / 2) % beat
        v += (rnd.random() * 2 - 1) * math.exp(-h * 70) * (0.12 if body else 0.07)   # the hats
        if body and not brk and k % 2 == 1:    # the snare, on 2 and 4
            v += (rnd.random() * 2 - 1) * math.exp(-u * 25) * 0.22 + math.sin(2 * math.pi * 190 * u) * math.exp(-u * 30) * 0.15
        if body and k % 32 == 0 and bar >= drop_bar:   # a crash on each phrase's first beat
            v += (rnd.random() * 2 - 1) * math.exp(-u * 3) * 0.12
        for m in (4, 4 * third, 6):            # the pad: the chord, two octaves up
            v += math.sin(2 * math.pi * root * m * t) * (0.03 if body else 0.05)
        v *= edge * 0.8
        s = max(-32767, min(32767, int(v * 32767)))
        out += struct.pack('<hh', s, s)
    with wave.open(path, 'wb') as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(bytes(out))
    print('wrote', path)


if __name__ == '__main__':
    d = sys.argv[1] if len(sys.argv) > 1 else 'example-tracks'
    os.makedirs(d, exist_ok=True)
    track(os.path.join(d, 'a-124bpm.wav'), 124, 55.0, True, 1, 4)
    track(os.path.join(d, 'b-124bpm.wav'), 124, 65.41, False, 2, 16)
    track(os.path.join(d, 'c-96bpm.wav'), 96, 49.0, True, 3, 4)
