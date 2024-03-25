#include "ClusterUnit.h"

void FSMProcess(hls::stream<Message> &inputStream,
		hls::stream<Edge>& treeEdges,
		hls::stream<Edge>& growEdges,
		hls::stream<Edge>& correctionEdges,
		hls::stream<bool>& outputChannel)
{
    Message command = inputStream.read();
#pragma HLS ARRAY_PARTITION variable=command.info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=command.info.syn_CPY type=complete

#pragma HLS ARRAY_PARTITION variable=command.info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=command.info.syn_CPY type=complete



    switch(command.TYPE)
    {
        case NEWINFO:
        	grow(command.info, growEdges);
            break;
        case STARTPEEL:
        	peel(command.info, treeEdges, correctionEdges);
        	outputChannel.write(true);
            break;
        default:
           break;
    }
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
    Vector<Edge, 5> expansions[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=expansions type=complete dim=2

    WRITING_EXPANSIONS:
    for(int i = 0; i < info.borders.getSize(); i++)
    {
#pragma HLS LOOP_FLATTEN off
    	utility(info.borders.at(i), expansions[i]);
        for (int j = 0; j < expansions[i].getSize(); j++)
        {
#pragma HLS LOOP_FLATTEN off
            expandedTo.write(expansions[i].at(j));
        }
    }
}

void peel(PUtoSend& info, hls::stream<Edge>& treeEdges, hls::stream<Edge>& corrections)
{
    Vector<ap_uint<BITSACCURACY>, 5> tree[SYN_LEN];
    bool syn_CPY[SYN_LEN];
    ap_uint<BITSACCURACY> toPeel = 0;
    ap_uint<BITSACCURACY> neighbor = 0;
    hls::stream<ap_uint<BITSACCURACY>> leafs("LEAFS");
#pragma HLS STREAM variable=leafs depth=CORR_LEN
    hls::stream<ap_uint<BITSACCURACY>> correctionsToCheck("CorrectionsToCheck");
#pragma HLS STREAM variable=correctionsToCheck depth=CORR_LEN*2

    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL
    	syn_CPY[i] = info.syn_CPY[i];
    }

    TREE_CREATION:
    while(!treeEdges.empty())
    {
#pragma HLS DEPENDENCE variable=tree type=intra dependent=false
        Edge e = treeEdges.read();

        tree[e.v].pushIn(e.u);

        tree[e.u].pushIn(e.v);
    }

    GETTING_LEAFS:
    for(int i = 0; i < SYN_LEN; i++)
    {
        if(tree[i].getSize() == 1)
        {
            leafs.write(i);
        }
    }

    TREE_PEELING:
    for(int i = 0; i < info.nodes_to_peel; i++)
    {
#pragma HLS DEPENDENCE variable=tree type=intra dependent=false
        toPeel = leafs.read();

        neighbor = tree[toPeel].at(0);

        tree[neighbor].eraseElement(toPeel);

        correctionsToCheck.write(toPeel);
        correctionsToCheck.write(neighbor);

        if(tree[neighbor].getSize() == 1)
        {
            leafs.write(neighbor);
        }
    }
    leafs.read();

    for(int i = 0; i < info.nodes_to_peel; i++)
    {
    	toPeel = correctionsToCheck.read();
    	neighbor = correctionsToCheck.read();

    	if(syn_CPY[toPeel])
		{
			Edge e = {toPeel, neighbor};
			corrections.write(e);
			syn_CPY[toPeel] = false;
			syn_CPY[neighbor] ^= true;
		}
    }
}
