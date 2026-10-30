// plan_join <a.wav> <b.wav> [model]: how Automix joins a into b: the kind of join, where it starts, how long it takes,
// and its curves. model: 1.0.0 or 1.1.0 (built in), or a .model file (models/automix-v1.2.0.model).
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "automix.hpp"
#include "wav.hpp"

static const char* kind(const automix::Plan& p) {
    if (p.off) return "none (Off)";
    if (p.drop && p.beatmatched) return "on the next track's drop, on the bars";
    if (p.drop) return "a bridge into the drop, the tempos apart";
    if (p.cut) return "a cut on a phrase, the tempos apart";
    if (p.structured) return "by the structure, on the bars";
    if (p.beatmatched) return p.onBars ? "on the beat and on the bars" : "on the beat";
    return p.join.effect == automix::Effect::Echo ? "timed, with an echo of the last beat" : "timed to the music (a smart crossfade)";
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: plan_join <a.wav> <b.wav> [1.0.0|1.1.0|<file>.model]\n"); return 2; }
    Wav a, b;
    std::string why;
    if (!readWav(argv[1], a, &why) || !readWav(argv[2], b, &why)) { fprintf(stderr, "%s\n", why.c_str()); return 1; }
    const mixan::Info ia = analyseWav(a), ib = analyseWav(b);
    const std::string m = argc > 3 ? argv[3] : "1.1.0";
    automix::Plan p;
    if (m.find(".model") != std::string::npos) {
        std::ifstream in(m, std::ios::binary);
        automix::ModelFile f;
        if (!f.parse(std::string(std::istreambuf_iterator<char>(in), {}), &why)) { fprintf(stderr, "%s: %s\n", m.c_str(), why.c_str()); return 1; }
        p = automix::plan(&ia, ia.seconds, &ib, 4, nullptr, f);
        printf("model     %s (from its file)\n", f.name.c_str());
    } else {
        automix::Model model;
        if (!automix::modelFromName("Automix V" + m, model)) { fprintf(stderr, "no built-in model %s\n", m.c_str()); return 1; }
        p = automix::plan(&ia, ia.seconds, &ib, 4, nullptr, model);
        printf("model     %s (built in)\n", automix::name(model));
    }
    printf("tracks    %.1f BPM into %.1f BPM", ia.bpm, ib.bpm);
    if (p.keyDistance >= 0) printf(", %s into %s (%d apart on the wheel)", mixan::camelot(ia.key).c_str(), mixan::camelot(ib.key).c_str(), p.keyDistance);
    printf("\njoin      %s\n", kind(p));
    printf("timing    from %.3f s of a; b from %.3f s; %.2f s long%s\n", p.startOut, p.offset, p.length,
           p.beats ? (", " + std::to_string(p.beats) + " beats").c_str() : "");
    printf("curves    volume %s, EQ %s, filter %s, effect %s%s\n", automix::name(p.join.volume), automix::name(p.join.eq),
           automix::name(p.join.filter), automix::name(p.join.effect), p.gate ? ", the next track in stabs" : "");
    return 0;
}
