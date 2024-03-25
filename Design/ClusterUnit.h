#ifndef QEC_UNION_FIND_CLUSTERUNIT_H
#define QEC_UNION_FIND_CLUSTERUNIT_H

#include "Defines.h"

void FSMProcess(hls::stream<Message> &inputStream,
		hls::stream<Edge>& treeEdges,
		hls::stream<Edge>& growEdges,
		hls::stream<Edge>& correctionEdges,
		hls::stream<bool>& outputChannel);

void grow(PUtoSend& info, hls::stream<Edge>& expandedTo);

void peel(PUtoSend& info, hls::stream<Edge>& treeEdges, hls::stream<Edge>& corrections);

#endif //QEC_UNION_FIND_CLUSTERUNIT_H
