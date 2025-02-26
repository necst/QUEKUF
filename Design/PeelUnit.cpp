#include "PeelUnit.h"

void FSMProcessPeel(hls::stream<Tree>& tree, hls::stream<Edge>& correctionEdges, hls::stream<ap_uint<BITSACCURACY>>& outputChannel)
{
    Tree t = tree.read();
#pragma HLS ARRAY_PARTITION variable=t.syn_CPY type=complete

    ap_uint<BITSACCURACY> counter = 0;

    peel(t, correctionEdges, counter);
    outputChannel.write(counter);
}


void correcting(ap_uint<BITSACCURACY> nodes_to_peel,
                hls::stream<Edge>& correctionsToCheck,
                bool syn_CPY[SYN_LEN],
                hls::stream<Edge>& corrections,
                ap_uint<BITSACCURACY>& counter)
{
    for(int i = 0; i < nodes_to_peel; i++)
    {
        Edge e = correctionsToCheck.read();

        if(syn_CPY[e.u])
        {
            corrections.write(e);
            counter++;
            syn_CPY[e.u] = false;
            syn_CPY[e.v] ^= true;
        }
    }
}

void traversal(ap_uint<BITSACCURACY> nodes_to_peel,
               hls::stream<Edge>& correctionsToCheck,
               Vector<ap_uint<BITSACCURACY>, 5> tree[SYN_LEN],
               hls::stream<ap_uint<BITSACCURACY>>& leafs)
{

    ap_uint<BITSACCURACY> toPeel = 0;
    ap_uint<BITSACCURACY> neighbor = 0;


    hls::stream<ap_uint<BITSACCURACY>> leafsQueue;
#pragma HLS STREAM variable=leafsQueue depth=CORR_LEN

    while(!leafs.empty())
    {
        leafsQueue.write(leafs.read());
    }

    for(int i = 0; i < nodes_to_peel; i++)
    {
#pragma HLS DEPENDENCE variable=tree type=intra dependent=false
        toPeel = leafsQueue.read();

        neighbor = tree[toPeel].at(0);

        tree[neighbor].eraseElement(toPeel);

        correctionsToCheck.write({toPeel, neighbor});

        if(tree[neighbor].getSize() == 1)
        {
            leafsQueue.write(neighbor);
        }
    }
    leafsQueue.read();
}

void treePeeling(ap_uint<BITSACCURACY> nodes_to_peel,
                 Vector<ap_uint<BITSACCURACY>, 5> tree[SYN_LEN],
                 bool syn_CPY[SYN_LEN],
                 hls::stream<ap_uint<BITSACCURACY>>& leafs,
                 hls::stream<Edge>& corrections,
                 ap_uint<BITSACCURACY>& counter)
{
    hls::stream<Edge> correctionsToCheck("CorrectionsToCheck");
#pragma HLS STREAM variable=correctionsToCheck depth=CORR_LEN*2

#pragma HLS DATAFLOW
    traversal(nodes_to_peel, correctionsToCheck, tree, leafs);
    correcting(nodes_to_peel, correctionsToCheck, syn_CPY, corrections, counter);
}

void peel(Tree t, hls::stream<Edge>& corrections, ap_uint<BITSACCURACY>& counter)
{
#pragma HLS INLINE
    Vector<ap_uint<BITSACCURACY>, 5> tree[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=tree type=complete
    bool syn_CPY[SYN_LEN];
    hls::stream<ap_uint<BITSACCURACY>> leafs("LEAFS");
#pragma HLS STREAM variable=leafs depth=CORR_LEN


    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL
        syn_CPY[i] = t.syn_CPY[i];
    }

    TREE_CREATION:
    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL

        ap_uint<4> vInfo = 0;
        ap_uint<BITSACCURACY> vPair = 0;
        //N,S,W,E
        vInfo[0] = t.treeEdges[i * 4 + 0];
        vInfo[1] = t.treeEdges[i * 4 + 1];
        vInfo[2] = t.treeEdges[i * 4 + 2];
        vInfo[3] = t.treeEdges[i * 4 + 3];

        if((vInfo & 0x1) > 0)
        {
            vPair = north(i);

            //t.treeEdges[vPair * 4 + 1] = 0;

            tree[i].pushIn(vPair);

            //tree[vPair].pushIn(i);
        }
        if((vInfo & 0x2) > 0)
        {
            vPair = south(i);

            //t.treeEdges[vPair * 4 + 0] = 0;

            tree[i].pushIn(vPair);

            //tree[vPair].pushIn(i);
        }
        if((vInfo & 0x4) > 0)
        {
            vPair = west(i);

            //t.treeEdges[vPair * 4 + 3] = 0;

            tree[i].pushIn(vPair);

            //tree[vPair].pushIn(i);
        }

        if((vInfo & 0x8) > 0)
        {
            vPair = east(i);

            //t.treeEdges[vPair * 4 + 2] = 0;

            tree[i].pushIn(vPair);

            //tree[vPair].pushIn(i);
        }


    }

    GETTING_LEAFS:
    for(int i = 0; i < SYN_LEN; i++)
    {
        if(tree[i].getSize() == 1)
        {
            leafs.write(i);
        }
    }

    treePeeling(t.nodes_to_peel, tree, syn_CPY, leafs, corrections, counter);

}
