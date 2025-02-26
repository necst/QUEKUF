#ifndef QEC_UNION_FIND_GROWUNIT_H
#define QEC_UNION_FIND_GROWUNIT_H

#include "Defines.h"

void FSMProcessGrow(hls::stream<Message> &inputStream,
                    hls::stream<Edge>& growEdges);

void grow(PUtoSend& info, hls::stream<Edge>& expandedTo);



#endif //QEC_UNION_FIND_GROWUNIT_HPP
