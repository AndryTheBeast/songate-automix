// songate-automix examples: reading and writing WAV files (16-bit, 24-bit, 32-bit integer or float PCM), nothing else, so
// the examples need no library. Songate itself decodes MP3, M4A and FLAC; the tools here use FFmpeg for those.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

struct Wav {
    int rate = 0, channels = 0;
    std::vector<float> samples;   // interleaved, -1..1
    long long frames() const { return channels ? (long long)(samples.size() / size_t(channels)) : 0; }
};

inline bool readWav(const std::string& path, Wav& w, std::string* why = nullptr) {
    auto fail = [&](const char* m) { if (why) *why = m; return false; };
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return fail("cannot open the file");
    std::vector<unsigned char> d;
    unsigned char buf[65536];
    for (size_t n; (n = fread(buf, 1, sizeof buf, f)) > 0;) d.insert(d.end(), buf, buf + n);
    fclose(f);
    auto u16 = [&](size_t at) { return unsigned(d[at] | d[at + 1] << 8); };
    auto u32 = [&](size_t at) { return unsigned(d[at] | d[at + 1] << 8 | d[at + 2] << 16 | unsigned(d[at + 3]) << 24); };
    if (d.size() < 12 || memcmp(d.data(), "RIFF", 4) || memcmp(d.data() + 8, "WAVE", 4)) return fail("not a WAV file");
    int format = 0, bits = 0;
    for (size_t at = 12; at + 8 <= d.size();) {
        const size_t len = u32(at + 4), body = at + 8;
        if (!memcmp(d.data() + at, "fmt ", 4) && len >= 16) {
            format = int(u16(body));
            w.channels = int(u16(body + 2));
            w.rate = int(u32(body + 4));
            bits = int(u16(body + 14));
            if (format == 0xFFFE && len >= 26) format = int(u16(body + 24));   // WAVE_FORMAT_EXTENSIBLE: its subformat
        } else if (!memcmp(d.data() + at, "data", 4)) {
            if (!w.channels || !bits) return fail("the data comes before its format");
            const size_t bytes = size_t(bits / 8), n = std::min(len, d.size() - body) / bytes;
            w.samples.resize(n);
            for (size_t i = 0; i < n; i++) {
                const unsigned char* p = d.data() + body + i * bytes;
                if (format == 3 && bits == 32) { float v; memcpy(&v, p, 4); w.samples[i] = v; }
                else if (bits == 16) w.samples[i] = int16_t(p[0] | p[1] << 8) / 32768.f;
                else if (bits == 24) w.samples[i] = float(int32_t(unsigned(p[0]) << 8 | unsigned(p[1]) << 16 | unsigned(p[2]) << 24) / 2147483648.0);
                else if (bits == 32) w.samples[i] = float(int32_t(unsigned(p[0]) | unsigned(p[1]) << 8 | unsigned(p[2]) << 16 | unsigned(p[3]) << 24) / 2147483648.0);
                else return fail("an unsupported sample format");
            }
            return true;
        }
        at = body + len + (len & 1);
    }
    return fail("no audio in it");
}

/** 16-bit stereo or mono WAV. */
inline bool writeWav(const std::string& path, const std::vector<float>& s, int rate, int channels) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint32_t data = uint32_t(s.size() * 2), riff = 36 + data, fmt = 16, br = uint32_t(rate * channels * 2), sr = uint32_t(rate);
    const uint16_t pcm = 1, ch = uint16_t(channels), align = uint16_t(channels * 2), bits = 16;
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmt, 4, 1, f);
    fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
    for (float v : s) {
        const int16_t x = int16_t(v >= 1 ? 32767 : v <= -1 ? -32768 : int(v * 32767.f));
        fwrite(&x, 2, 1, f);
    }
    fclose(f);
    return true;
}

/** A track's analysis, as Songate makes it while it measures loudness: the whole track pushed through mixan::Analyzer. */
#include "mixan.hpp"
inline mixan::Info analyseWav(const Wav& w) {
    mixan::Analyzer a;
    a.begin(w.rate, w.channels);
    const size_t block = size_t(4096 * w.channels);
    for (size_t at = 0; at < w.samples.size(); at += block)
        a.push(w.samples.data() + at, int(std::min(block, w.samples.size() - at) / size_t(w.channels)));
    return a.finish();
}
