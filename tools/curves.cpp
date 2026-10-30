// curves <pairs.tsv> <out.json>: each join's curves as Songate's mixer computes them (automix::gainsAt), for the diagrams
// (docs/diagrams/make_diagrams.py). pairs.tsv: "label<TAB>model<TAB>a's analysis<TAB>b's analysis" (mixan::Info::pack();
// model: 1.0.0, 1.1.0 or a .model file under models/). Only the analyses' numbers: no audio, no names.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "automix.hpp"

static const char* kindOf(const automix::Plan& p) {
    if (p.drop && p.beatmatched) return p.gate ? "drop-stabs" : "drop";
    if (p.drop) return "bridge";
    if (p.cut) return "cut";
    if (p.structured) return p.gate ? "structure-stabs" : "structure";
    if (p.beatmatched) return p.join.volume == automix::Volume::BothUp ? "beat-blend" : "beat-smooth";
    if (p.join.effect == automix::Effect::Echo) return "echo";
    return p.join.filter == automix::Filter::LowPassIn ? "rise" : "fade";
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: curves <pairs.tsv> <out.json> [models folder]\n"); return 2; }
    const std::string models = argc > 3 ? argv[3] : "models";
    std::ifstream in(argv[1], std::ios::binary);
    std::ofstream out(argv[2], std::ios::binary);
    out << "[\n";
    bool first = true;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
        if (f.size() < 4) continue;
        mixan::Info a, b;
        if (!mixan::Info::unpack(f[2], a) || !mixan::Info::unpack(f[3], b)) { fprintf(stderr, "%s: not analyses\n", f[0].c_str()); continue; }
        automix::Plan p;
        std::string model;
        if (f[1].find(".model") != std::string::npos) {
            std::ifstream m(models + "/" + f[1], std::ios::binary);
            automix::ModelFile mf;
            if (!mf.parse(std::string(std::istreambuf_iterator<char>(m), {}))) { fprintf(stderr, "%s: no model %s\n", f[0].c_str(), f[1].c_str()); continue; }
            p = automix::plan(&a, a.seconds, &b, 4, nullptr, mf);
            model = mf.name;
        } else {
            automix::Model mm;
            if (!automix::modelFromName("Automix V" + f[1], mm)) continue;
            p = automix::plan(&a, a.seconds, &b, 4, nullptr, mm);
            model = automix::name(mm);
        }
        out << (first ? "" : ",\n") << "{\"label\": \"" << f[0] << "\", \"model\": \"" << model << "\", \"kind\": \"" << kindOf(p)
            << "\", \"length\": " << p.length << ", \"beats\": " << p.beats << ", \"bpm\": " << p.bpm << ", \"bpmA\": " << a.bpm
            << ", \"bpmB\": " << b.bpm << ", \"cross\": " << p.cross << ", \"bassCut\": " << p.bassCut << ", \"gate\": " << p.gate
            << ", \"volume\": \"" << automix::name(p.join.volume) << "\", \"eq\": \"" << automix::name(p.join.eq) << "\", \"filter\": \""
            << automix::name(p.join.filter) << "\", \"effect\": \"" << automix::name(p.join.effect) << "\", \"points\": [";
        first = false;
        constexpr int N = 600;
        for (int i = 0; i <= N; i++) {
            automix::Gains g;
            automix::gainsAt(p, double(i) / N, g);
            char b2[256];
            snprintf(b2, sizeof b2, "%s[%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.1f,%.1f,%.4f]", i ? "," : "", g.bandIn[0], g.bandOut[0],
                     g.bandIn[1], g.bandOut[1], g.bandIn[2], g.bandOut[2], g.lpIn, g.hpOut, g.wet);
            out << b2;   // the bass, mid and treble of each (volume × EQ), the filters' cut-offs (Hz), the echo
        }
        out << "]}";
        printf("%-28s %-16s %s, %.1f s\n", f[0].c_str(), kindOf(p), model.c_str(), p.length);
    }
    out << "\n]\n";
    return 0;
}
