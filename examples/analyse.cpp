// analyse <track.wav>: what Automix measures in a track (mixan::Info), and the line Songate keeps for it in mix.tsv.
#include <cstdio>

#include "mixan.hpp"
#include "wav.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: analyse <track.wav>\n"); return 2; }
    Wav w;
    std::string why;
    if (!readWav(argv[1], w, &why)) { fprintf(stderr, "%s: %s\n", argv[1], why.c_str()); return 1; }
    const mixan::Info i = analyseWav(w);
    char fade[64];
    snprintf(fade, sizeof fade, i.fadeStart < i.end - 0.5f ? ", fading out from %.2f s" : ", ending abruptly", i.fadeStart);
    printf("length       %.2f s, music from %.2f to %.2f s%s\n", i.seconds, i.start, i.end, fade);
    printf("tempo        %.1f BPM, beat grid sure %.2f\n", i.bpm, i.confidence);
    printf("bars         first downbeat %.3f s (sure %.2f), last %.3f s (sure %.2f), phrases from bar %d\n", i.barIn, i.barSureIn,
           i.barOut, i.barSureOut, i.phraseAt);
    if (i.key >= 0) printf("key          %s (%s on the Camelot wheel), sure %.2f\n", mixan::keyName(i.key).c_str(), mixan::camelot(i.key).c_str(), i.keySure);
    printf("level        %.1f dB (the body); bass %.0f %% at the start, %.0f %% at the end\n", i.level, i.bassIn * 100, i.bassOut * 100);
    if (i.dropAt >= 0) printf("drop         at %.2f s, the bass up %.1f dB\n", i.dropAt, i.dropJump);
    printf("bar levels   %s\nbar bass     %s\n", i.bars.c_str(), i.bass.c_str());
    printf("\nmix.tsv      %s\n", i.pack().c_str());
    return 0;
}
