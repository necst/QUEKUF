#ifndef QEC_UNION_FIND_DECODERMAIN_H
#define QEC_UNION_FIND_DECODERMAIN_H

#include "Defines.h"

extern "C"
{
    void decoderTop(bool syndrome[SYN_LEN], bool correction[CORR_LEN], int64_t* cc);
};


void readSyn(bool syndrome[SYN_LEN], hls::stream<ap_uint<BITSACCURACY>>& SynBits, hls::stream<bool>& syndrome_cpy);

void initPU(hls::stream<ap_uint<BITSACCURACY>>& SynBits, hls::stream<PU>& infoPU, hls::stream<bool>& syndrome_cpy);

void translation(hls::stream<Edge>& correctionEdges, bool correction[CORR_LEN]);

#endif //QEC_UNION_FIND_DECODERMAIN_H
