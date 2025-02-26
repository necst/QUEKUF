#ifndef QEC_UNION_FIND_PEELUNIT_H
#define QEC_UNION_FIND_PEELUNIT_H
#include "Defines.h"


void FSMProcessPeel(hls::stream<Tree>& tree,
                hls::stream<Edge>& correctionEdges,
                hls::stream<ap_uint<BITSACCURACY>>& outputChannel);


void peel(Tree t, hls::stream<Edge>& corrections, ap_uint<BITSACCURACY>& counter);

#endif //QEC_UNION_FIND_PEELUNIT_HPP
