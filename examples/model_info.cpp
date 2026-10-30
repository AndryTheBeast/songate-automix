// model_info <file>.model: reads a model file as Songate does (automix::ModelFile) and writes it back out: whether this
// build can plan it, and every number in it. Songate refuses a model of a newer format or a newer analysis.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "automix.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: model_info <file>.model\n"); return 2; }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
    automix::ModelFile m;
    std::string why;
    if (!m.parse(std::string(std::istreambuf_iterator<char>(in), {}), &why)) { fprintf(stderr, "not plannable here: %s\n", why.c_str()); return 1; }
    printf("# %s: format %d (this build plans up to %d), Songate %s or newer, analysis version %d (this build: %d)\n", m.name.c_str(),
           m.format, automix::ModelFile::Format, m.app.c_str(), m.mixan, mixan::Version);
    printf("%s", m.text().c_str());
    return 0;
}
