"""The README's diagrams, as SVG: each kind of join from the mixer's own numbers (curves.json, made by build/curves from
pairs.tsv), and the flow charts (the analysis, training, a model's way to the console).

    ./build/curves docs/diagrams/pairs.tsv docs/diagrams/curves.json models
    python docs/diagrams/make_diagrams.py
"""
import json, math, os

HERE = os.path.dirname(os.path.abspath(__file__))
BG, CARD, LINE, TEXT, TEXT2, TEXT3 = '#0f1117', '#171a22', '#2a2f3b', '#eef0f6', '#a7adbd', '#6c7386'
OUT_C, IN_C, ACC2 = '#ffad7a', '#8fb3ff', '#c3a2ff'   # the outgoing track, the incoming one, Songate's second accent
FONT = "font-family='Inter, Segoe UI, Helvetica, Arial, sans-serif'"

TITLES = {
    'drop': ('On the drop', 'Tempos that meet. The next track enters bass-less over 8 bars, the bass swaps hard on the drop, the outgoing track leaves a bar later.'),
    'drop-stabs': ('On the drop, with stabs (V1.2.0)', 'The next track arrives in short stabs on every beat until its drop, the outgoing track dipping under each; then the bass swaps.'),
    'bridge': ('A bridge between tempos', "Tempos too far apart to beat-match: the next track's bass-less lead-in plays under this one's last bars, then its drop."),
    'structure': ('By the structure', "On the bars of both: the next track's intro under this one's outro, its first loud section on the bar where the outro begins."),
    'structure-stabs': ('By the structure, with stabs (V1.2.0)', 'The same join, the incoming intro played in stabs on every beat until the crossover.'),
    'beat-blend': ('On the beat: a blend', 'Both up and a full EQ swap: the treble changes hands a little before the crossover, the bass on it.'),
    'beat-smooth': ('On the beat: short', 'Under 16 beats (8 in V1.2.0): an equal-power crossfade with the bass swapped in the middle.'),
    'fade': ('Timed to the music', "No common beat: a crossfade through this track's own fade-out, the bass handed over in the middle."),
    'echo': ('An echo out', "An abrupt end without a fade: the last full beat repeats and dies away while the next track comes in."),
}


def path(points, idx, x0, y0, w, h):
    n = len(points) - 1
    return ' '.join(('M' if i == 0 else 'L') + f'{x0 + w * i / n:.1f},{y0 + h * (1 - points[i][idx]):.1f}' for i in range(n + 1))


def join_svg(j):
    W, H = 780, 330
    x0, y0, w, h = 64, 96, 690, 168
    title, sub = TITLES[j['kind']]
    meta = f"{j['model']} · {j['length']:.1f} s" + (f" · {j['beats']} beats at {j['bpm']:.0f} BPM" if j['beats'] else
                                                   f" · {j['bpmA']:.0f} into {j['bpmB']:.0f} BPM" if j['bpmA'] and j['bpmB'] else '')
    o = [f"<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}' {FONT}>",
         f"<rect width='{W}' height='{H}' rx='18' fill='{CARD}'/>",
         f"<text x='28' y='38' font-size='19' font-weight='700' fill='{TEXT}'>{title}</text>",
         f"<text x='28' y='62' font-size='12.5' fill='{TEXT2}'>{sub}</text>",
         f"<text x='{W - 28}' y='38' font-size='12.5' fill='{TEXT3}' text-anchor='end'>{meta}</text>"]
    # grid: levels 0, 0.5, 1 and the seconds
    for lv in (0, 0.5, 1):
        y = y0 + h * (1 - lv)
        o.append(f"<line x1='{x0}' y1='{y:.1f}' x2='{x0 + w}' y2='{y:.1f}' stroke='{LINE}' stroke-width='1'/>")
        o.append(f"<text x='{x0 - 10}' y='{y + 4:.1f}' font-size='11' fill='{TEXT3}' text-anchor='end'>{lv:g}</text>")
    L = j['length']
    step = 1 if L <= 8 else 2 if L <= 16 else 4
    s = 0
    while s <= L + 1e-6:
        x = x0 + w * s / L
        o.append(f"<line x1='{x:.1f}' y1='{y0 + h}' x2='{x:.1f}' y2='{y0 + h + 5}' stroke='{TEXT3}'/>")
        o.append(f"<text x='{x:.1f}' y='{y0 + h + 19}' font-size='11' fill='{TEXT3}' text-anchor='middle'>{s:g} s</text>")
        s += step
    if 0 < j['cross'] < 1:
        x = x0 + w * j['cross']
        o.append(f"<line x1='{x:.1f}' y1='{y0 - 8}' x2='{x:.1f}' y2='{y0 + h}' stroke='{ACC2}' stroke-width='1.2' stroke-dasharray='4 4'/>")
        o.append(f"<text x='{x + 6:.1f}' y='{y0 - 10}' font-size='11' fill='{ACC2}'>crossover</text>")
    if j['bassCut'] > 0 and j['eq'].startswith('Bass cut'):
        x = x0 + w * j['bassCut']
        o.append(f"<line x1='{x:.1f}' y1='{y0 - 8}' x2='{x:.1f}' y2='{y0 + h}' stroke='{OUT_C}' stroke-width='1' stroke-dasharray='2 4' opacity='0.8'/>")
        o.append(f"<text x='{x - 6:.1f}' y='{y0 - 10}' font-size='11' fill='{OUT_C}' text-anchor='end'>bass cut</text>")
    pts = j['points']
    # the echo's tail, shaded
    if any(p[8] > 0 for p in pts):
        o.append(f"<path d='{path(pts, 8, x0, y0, w, h)} L{x0 + w},{y0 + h} L{x0},{y0 + h} Z' fill='{OUT_C}' opacity='0.12'/>")
    for idx, col, width, dash in ((1, OUT_C, 3, ''), (3, OUT_C, 1.6, ''), (5, OUT_C, 1.4, "stroke-dasharray='5 4'"),
                                  (0, IN_C, 3, ''), (2, IN_C, 1.6, ''), (4, IN_C, 1.4, "stroke-dasharray='5 4'")):
        o.append(f"<path d='{path(pts, idx, x0, y0, w, h)}' fill='none' stroke='{col}' stroke-width='{width}' stroke-linejoin='round' {dash}/>")
    # the legend
    lx, ly = 28, H - 22
    for label, col in (('outgoing track', OUT_C), ('incoming track', IN_C)):
        o.append(f"<rect x='{lx}' y='{ly - 9}' width='12' height='12' rx='3' fill='{col}'/>")
        o.append(f"<text x='{lx + 18}' y='{ly + 1}' font-size='12' fill='{TEXT2}'>{label}</text>")
        lx += 132
    for label, width, dash in (('bass', 3, ''), ('mids', 1.6, ''), ('treble', 1.4, "stroke-dasharray='5 4'")):
        o.append(f"<line x1='{lx}' y1='{ly - 3}' x2='{lx + 22}' y2='{ly - 3}' stroke='{TEXT2}' stroke-width='{width}' {dash}/>")
        o.append(f"<text x='{lx + 28}' y='{ly + 1}' font-size='12' fill='{TEXT2}'>{label}</text>")
        lx += 84
    extra = []
    if j['filter'] != 'None': extra.append('filter: ' + j['filter'].lower())
    if any(p[8] > 0 for p in pts): extra.append('shaded: the echo')
    if extra:
        o.append(f"<text x='{W - 28}' y='{ly + 1}' font-size='12' fill='{TEXT3}' text-anchor='end'>{' · '.join(extra)}</text>")
    o.append('</svg>')
    return '\n'.join(o)


def box(o, x, y, w, h, title, lines, col=IN_C):
    o.append(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='12' fill='{BG}' stroke='{col}' stroke-opacity='0.55'/>")
    o.append(f"<text x='{x + 14}' y='{y + 24}' font-size='14' font-weight='700' fill='{TEXT}'>{title}</text>")
    for i, l in enumerate(lines):
        o.append(f"<text x='{x + 14}' y='{y + 44 + i * 17}' font-size='11.5' fill='{TEXT2}'>{l}</text>")


def arrow(o, x1, y1, x2, y2, col=TEXT3):
    a = math.atan2(y2 - y1, x2 - x1)
    o.append(f"<line x1='{x1}' y1='{y1}' x2='{x2}' y2='{y2}' stroke='{col}' stroke-width='1.6'/>")
    for s in (-1, 1):
        o.append(f"<line x1='{x2}' y1='{y2}' x2='{x2 - 8 * math.cos(a + s * 0.45):.1f}' y2='{y2 - 8 * math.sin(a + s * 0.45):.1f}' stroke='{col}' stroke-width='1.6'/>")


def frame(W, H, title, sub):
    return [f"<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}' {FONT}>",
            f"<rect width='{W}' height='{H}' rx='18' fill='{CARD}'/>",
            f"<text x='28' y='38' font-size='19' font-weight='700' fill='{TEXT}'>{title}</text>",
            f"<text x='28' y='62' font-size='12.5' fill='{TEXT2}'>{sub}</text>"]


def analysis_svg():
    o = frame(780, 420, 'The analysis (mixan)', 'One pass over the decoded track, in the same decode as the loudness measurement; kept as a line of mix.tsv.')
    box(o, 28, 88, 160, 92, 'Decode', ['the track as it is,', 'mono, cut down to', '≈ 11 kHz'])
    box(o, 220, 88, 200, 92, 'Every 128 samples', ['a 1024-point spectrum:', 'onset strength (flux),', 'loudness, bass share'])
    box(o, 452, 88, 300, 92, 'Every 4 hops', ['the chroma: 12 pitch classes, G♯3–G♯6,', 'each semitone weighed alike', '(and the bass\'s, 52–208 Hz)'])
    arrow(o, 188, 134, 218, 134)
    arrow(o, 420, 134, 450, 134)
    box(o, 28, 214, 230, 118, 'Tempo and beats', ['autocorrelation with harmonics,', 'a prior round 120 BPM; beats by', 'dynamic programming (Ellis 2007);', 'a grid at each end'], ACC2)
    box(o, 274, 214, 230, 118, 'Bars, phrases, drops', ['downbeats from the bars\' accents,', '8-bar phrases; each bar\'s level and', 'bass (a digit each); the drop where', 'the bass comes back in full'], ACC2)
    box(o, 520, 214, 232, 118, 'Key and ends', ['the best of 24 key profiles', '(start, end, whole) → Camelot;', 'where the music starts, ends,', 'fades out, how loud the body is'], ACC2)
    for x in (143, 389, 636):
        arrow(o, x, 182, x, 212)
    o.append(f"<rect x='28' y='352' width='724' height='44' rx='10' fill='{BG}'/>")
    o.append(f"<text x='42' y='379' font-size='12' fill='{TEXT2}' font-family='Consolas, monospace'>path ⇥ 4,124.000,1.000,0.7324,233.871,… (mixan Version 4: a new analysis raises it)</text>")
    o.append('</svg>')
    return '\n'.join(o)


def training_svg():
    o = frame(780, 300, 'Training a model', 'Rounds of 24 blind demos from a real library, rated 0 to 10, until each kind of join has settled.')
    steps = [('mixtrain round', ['picks joins of each kind', 'from the library\'s folders,', 'renders 3 versions each']),
             ('rate.html', ['blind: the songs shown,', 'never the version;', '0-10 and a note each']),
             ('the findings', ['each version against the', 'reference, kind by kind;', 'what to test next']),
             ('freeze', ['the numbers that won,', 'written into a new model', 'that never changes'])]
    for i, (t, ls) in enumerate(steps):
        box(o, 28 + i * 186, 92, 168, 104, t, ls, IN_C if i < 3 else ACC2)
        if i < 3:
            arrow(o, 196 + i * 186, 144, 212 + i * 186, 144)
    o.append(f"<path d='M 578 200 C 578 250, 112 250, 112 200' fill='none' stroke='{TEXT3}' stroke-width='1.4' stroke-dasharray='4 4'/>")
    o.append(f"<text x='345' y='262' font-size='12' fill='{TEXT3}' text-anchor='middle'>the next round starts from the round's best (V1.1.0: rounds 1-4 · V1.2.0: round 5)</text>")
    o.append('</svg>')
    return '\n'.join(o)


def ota_svg():
    o = frame(780, 300, "A model's way to the console", 'A new model that only changes numbers needs no new Songate.')
    box(o, 28, 92, 196, 108, 'This repository', ['models/automix-v1.2.0.model', 'tools/mkmodels.py writes', 'songate-automix-models.json'])
    box(o, 292, 92, 196, 108, 'A GitHub release', ['the manifest (name, file, size,', 'SHA-256, format, lowest', 'Songate, analysis) and files'])
    box(o, 556, 92, 196, 108, 'Songate', ['Settings › Updates › Automix', 'models: checked, downloaded,', 'its checksum checked'], ACC2)
    arrow(o, 224, 146, 290, 146)
    arrow(o, 488, 146, 554, 146)
    o.append(f"<text x='654' y='226' font-size='12' fill='{TEXT2}' text-anchor='middle'>config/gatefold-nx/models/</text>")
    o.append(f"<text x='654' y='246' font-size='12' fill='{TEXT2}' text-anchor='middle'>the app and the background player</text>")
    o.append(f"<text x='654' y='266' font-size='12' fill='{TEXT2}' text-anchor='middle'>plan with the same file</text>")
    o.append('</svg>')
    return '\n'.join(o)


if __name__ == '__main__':
    curves = json.load(open(os.path.join(HERE, 'curves.json'), encoding='utf-8'))
    for j in curves:
        open(os.path.join(HERE, f"join-{j['kind']}.svg"), 'w', encoding='utf-8', newline='\n').write(join_svg(j))
    for name, f in (('analysis', analysis_svg), ('training', training_svg), ('over-the-air', ota_svg)):
        open(os.path.join(HERE, name + '.svg'), 'w', encoding='utf-8', newline='\n').write(f())
    print('wrote', len(curves) + 3, 'diagrams')
