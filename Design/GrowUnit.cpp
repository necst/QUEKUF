#include "GrowUnit.h"

void FSMProcessGrow(hls::stream<Message> &inputStream,
		            hls::stream<Edge>& growEdges)
{
    Message command = inputStream.read();

    grow(command.info, growEdges);
}

void utility(ap_uint<BITSACCURACY> vertex, Vector<Edge,5>& expansion)
{
    Vector<ap_uint<BITSACCURACY>, 5> connections = vertex_connections(vertex);
#pragma HLS ARRAY_PARTITION variable=connections.array type=complete
    GROW_EXPANSIONS:
    for(int i = 0; i < vertex_connection_count(vertex); i++)
    {
#pragma HLS UNROLL
    	Edge e0{};
    	e0.u = std::min(vertex, connections.at(i));
		e0.v = std::max(vertex, connections.at(i));
		expansion.pushIn(e0);
    }
}

void grow(PUtoSend& info, hls::stream<Edge>& expandedTo)
{
#pragma HLS INLINE
    Vector<Edge, 5> expansions[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=expansions type=complete dim=2

    WRITING_EXPANSIONS:
    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS LOOP_FLATTEN off
        if(info.borders[INDEX(i)])
        {
            utility(i, expansions[i]);
            for (int j = 0; j < 4; j++)
            {
#pragma HLS LOOP_FLATTEN off
                expandedTo.write(expansions[i].at(j));
            }
        }
    }
}


