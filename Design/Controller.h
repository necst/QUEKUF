#ifndef QEC_UNION_FIND_CONTROLLER_H
#define QEC_UNION_FIND_CONTROLLER_H

#include "Defines.h"


void controllerProcess(hls::stream<PU>& infoPE,
                       hls::stream<Edge>& correctionEdges);

void syncPU(hls::stream<Edge>& fuseEdges,
            hls::stream<ap_uint<BITSACCURACY>>& status,
            hls::stream<bool>& outputChannel,
            hls::stream<bool>& done,
            hls::stream<Edge>& growEdges,
			hls::stream<Edge>& corrEdges,
            hls::stream<Edge>& correctionEdges,
            hls::stream<ap_uint<BITSACCURACY>>& totalSyncsChannel,
            bool& doneW);

void fusion(hls::stream<Edge>& fuseEdges,
		hls::stream<bool>& done,
		hls::stream<ap_uint<BITSACCURACY>>& n_fusions,
		PU graphStatus[SYN_LEN],
		bool& odds);

void mainLoop(hls::stream<Edge>& correctionEdges,
              PU graphStatusCPY[SYN_LEN],
              PU graphStatus[SYN_LEN],
              bool& oddsR,
              bool& oddsW,
              bool& doneW
);
#endif //QEC_UNION_FIND_CONTROLLER_H
