// render_join <a.wav> <b.wav> <out.wav> [model]: the join as Songate plays it, 12 s either side (automix::render: the
// same Fader, EQ, filters, echo and limiter as Songate's player). The two files must share a sample rate (Songate resamples
// both to the console's 48 kHz before they meet; resampling is left out of the examples).
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "automix.hpp"
#include "wav.hpp"

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: render_join <a.wav> <b.wav> <out.wav> [1.0.0|1.1.0|<file>.model]\n"); return 2; }
    Wav a, b;
    std::string why;
    if (!readWav(argv[1], a, &why) || !readWav(argv[2], b, &why)) { fprintf(stderr, "%s\n", why.c_str()); return 1; }
    if (a.rate != b.rate) { fprintf(stderr, "the two files have different sample rates (%d, %d)\n", a.rate, b.rate); return 1; }
    const mixan::Info ia = analyseWav(a), ib = analyseWav(b);
    const std::string m = argc > 4 ? argv[4] : "1.1.0";
    automix::Plan p;
    if (m.find(".model") != std::string::npos) {
        std::ifstream in(m, std::ios::binary);
        automix::ModelFile f;
        if (!f.parse(std::string(std::istreambuf_iterator<char>(in), {}), &why)) { fprintf(stderr, "%s\n", why.c_str()); return 1; }
        p = automix::plan(&ia, ia.seconds, &ib, 4, nullptr, f);
    } else {
        automix::Model model = automix::Latest;
        automix::modelFromName("Automix V" + m, model);
        p = automix::plan(&ia, ia.seconds, &ib, 4, nullptr, model);
    }
    // readers handing out 16-bit stereo, as Songate's decoders do: a from where the render starts, b from the plan's offset
    auto reader = [](const Wav& w, double from) {
        return [&w, at = (long long)(from * w.rate)](int16_t* out, int n) mutable {
            int got = 0;
            for (; got < n && at < w.frames(); got++, at++)
                for (int c = 0; c < 2; c++) {
                    const float v = w.samples[size_t(at * w.channels + (w.channels > 1 ? c : 0))];
                    out[got * 2 + c] = int16_t(v >= 1 ? 32767 : v <= -1 ? -32768 : int(v * 32767.f));
                }
            return got;
        };
    };
    const double pre = std::min(12.0, p.startOut);
    std::vector<float> out;
    automix::render(p, a.rate, pre, 12, reader(a, p.startOut - pre), reader(b, p.offset),
                    [&](const float* x, int n) { out.insert(out.end(), x, x + n * 2); });
    if (!writeWav(argv[3], out, a.rate, 2)) { fprintf(stderr, "cannot write %s\n", argv[3]); return 1; }
    printf("%.2f s join at %.2f s of a, rendered from %.2f s before it: %s (%.1f s)\n", p.length, p.startOut, pre, argv[3],
           double(out.size() / 2) / a.rate);
    return 0;
}
