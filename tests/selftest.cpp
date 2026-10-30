// selftest [models folder]: synthetic tracks of known tempo through the analysis and the planner, and the model files.
// - the analysis finds each track's tempo (within 1 BPM) and a sure beat grid;
// - two tracks at the same tempo are joined on the beat, two far apart are not;
// - models/automix-v1.1.0.model plans every pair exactly as the built-in Automix V1.1.0 (every field);
// - a model file reads back to the same text; a newer format and a newer analysis are refused.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "automix.hpp"

static int fails = 0;
static void check(bool ok, const std::string& what) {
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what.c_str());
    fails += !ok;
}

/** A dance loop: a kick on every beat, a bass note under it, hats between, 8-bar phrases (a kick-less break every 4th). */
static mixan::Info track(double bpm, double seconds, int rootHz, unsigned seed) {
    const int rate = 44100;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> noise(-1, 1);
    std::vector<float> s(size_t(seconds * rate) * 2);
    const double beat = 60.0 / bpm;
    for (size_t i = 0; i < s.size() / 2; i++) {
        const double t = double(i) / rate, intro = std::min(1.0, t / 8), outro = std::min(1.0, (seconds - t) / 8);
        const int k = int(t / beat), bar = k / 4;
        const double u = t - k * beat;   // time since the beat
        const bool brk = (bar / 8) % 4 == 3;   // every fourth phrase without its kick
        float v = 0;
        if (!brk) v += float(std::sin(2 * M_PI * (50 + 80 * std::exp(-u * 30)) * u) * std::exp(-u * 9)) * 0.8f;   // kick
        v += float(std::sin(2 * M_PI * rootHz * t) * 0.25 * (brk ? 0.2 : 1));                                     // bass
        const double h = std::fmod(t + beat / 2, beat);
        v += noise(rng) * float(std::exp(-h * 60)) * 0.15f;                                                       // hat
        v += float(std::sin(2 * M_PI * rootHz * 4 * t) * 0.05);                                                   // a pad
        v *= float(std::min(intro, outro));
        s[i * 2] = s[i * 2 + 1] = v * 0.7f;
    }
    mixan::Analyzer a;
    a.begin(rate, 2);
    for (size_t at = 0; at < s.size(); at += 8192) a.push(s.data() + at, int(std::min<size_t>(8192, s.size() - at) / 2));
    return a.finish();
}

static bool samePlan(const automix::Plan& x, const automix::Plan& y) {
    return x.off == y.off && x.startOut == y.startOut && x.offset == y.offset && x.length == y.length && x.beatmatched == y.beatmatched &&
           x.bpm == y.bpm && x.beats == y.beats && x.join == y.join && x.echoAt == y.echoAt && x.echoDelay == y.echoDelay &&
           x.onBars == y.onBars && x.structured == y.structured && x.drop == y.drop && x.cut == y.cut && x.bassCut == y.bassCut &&
           x.cross == y.cross && x.keyDistance == y.keyDistance && x.inLevel == y.inLevel && x.lpFrom == y.lpFrom &&
           x.outPow == y.outPow && x.cutFade == y.cutFade && x.gate == y.gate;
}

static std::string slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

int main(int argc, char** argv) {
    const std::string models = argc > 1 ? argv[1] : "models";
    const double tempos[] = { 96, 120, 124, 128, 140 };
    std::vector<mixan::Info> t;
    for (int i = 0; i < 5; i++) {
        t.push_back(track(tempos[i], 150, 55 + 7 * i, unsigned(i + 1)));
        const mixan::Info& x = t.back();
        char b[160];
        snprintf(b, sizeof b, "%.0f BPM track: found %.2f BPM, beat grid sure %.2f", tempos[i], x.bpm, x.confidence);
        check(x.valid && std::fabs(x.bpm - tempos[i]) < 1 && x.confidence >= 0.5f, b);
    }
    const automix::Plan same = automix::plan(&t[2], t[2].seconds, &t[3], 4, nullptr, automix::Model::V1_1_0);
    check(!same.beatmatched || std::fabs(t[2].bpm - t[3].bpm) / t[3].bpm < 0.05, "124 into 128: on the beat only where the tempos meet");
    const automix::Plan equal = automix::plan(&t[3], t[3].seconds, &t[3], 4, nullptr, automix::Model::V1_1_0);
    check(equal.beatmatched, "128 into 128: on the beat");
    const automix::Plan apart = automix::plan(&t[0], t[0].seconds, &t[4], 4, nullptr, automix::Model::V1_1_0);
    check(!apart.beatmatched, "96 into 140: not on the beat (no tempo change)");

    automix::ModelFile v11, v12;
    check(v11.parse(slurp(models + "/automix-v1.1.0.model")), "automix-v1.1.0.model reads");
    check(v12.parse(slurp(models + "/automix-v1.2.0.model")), "automix-v1.2.0.model reads");
    int n = 0, same11 = 0, gated = 0;
    for (size_t a = 0; a < t.size(); a++)
        for (size_t b = 0; b < t.size(); b++) {
            n++;
            same11 += samePlan(automix::plan(&t[a], t[a].seconds, &t[b], 4, nullptr, v11),
                               automix::plan(&t[a], t[a].seconds, &t[b], 4, nullptr, automix::Model::V1_1_0));
            gated += automix::plan(&t[a], t[a].seconds, &t[b], 4, nullptr, v12).gate != 0;
        }
    check(same11 == n, "V1.1.0 from its file plans as the built-in V1.1.0 (" + std::to_string(same11) + " of " + std::to_string(n) + " pairs)");
    check(gated > 0, "V1.2.0 plays its stabs on the drops it finds (" + std::to_string(gated) + " of " + std::to_string(n) + " pairs)");
    automix::ModelFile back;
    check(back.parse(v12.text()) && back.text() == v12.text(), "a model file reads back to the same text");
    std::string why;
    std::string newer = v12.text();
    newer.replace(0, newer.find('\n'), "songate-automix-model 99");
    const bool newerOk = automix::ModelFile().parse(newer, &why);
    check(!newerOk, "a newer format is refused: " + why);
    std::string later = v12.text();
    later.replace(later.find("mixan "), later.find('\n', later.find("mixan ")) - later.find("mixan "), "mixan 99");
    const bool laterOk = automix::ModelFile().parse(later, &why);
    check(!laterOk, "a newer analysis is refused: " + why);
    printf(fails ? "\n%d FAILED\n" : "\nall good\n", fails);
    return fails ? 1 : 0;
}
