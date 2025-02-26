#include "Controller.h"
#include "GrowUnit.h"
#include "PeelUnit.h"
#include "Defines.h"
#include "hls_print.h"

//#define CBUILD

bool globalSyndrome[SYN_LEN];

void sendInfo(hls::stream<Message>& inputStream,
              hls::stream<ap_uint<BITSACCURACY>>& status,
              hls::stream<ap_uint<BITSACCURACY>>& n_expansions,
			  hls::stream<ap_uint<BITSACCURACY>>& n_fusions,
              PU graphStatus[SYN_LEN],
              bool& odds,
              hls::stream<Tree>& trees)
{
    ap_uint<BITSACCURACY> globalStatus = GROWING;
    ap_uint<BITSACCURACY> totalSyncs = 0;
WRITING_TO_PU:
    for(int i = 0; i < SYN_LEN; i++)
    {
        Message msg{};
        switch(graphStatus[i].status)
        {
        case WAITING:
        	if(odds == 0)
			{
                Tree t{};
                t.nodes_to_peel = graphStatus[i].nodes_to_peel;
                t.treeEdges = graphStatus[i].treeEdges;
                SYN_CPY_PEEL:
                for(int j = 0; j < SYN_LEN; j++)
                {
#pragma HLS UNROLL
                    t.syn_CPY[j] = globalSyndrome[j];
                }
                trees.write(t);

                graphStatus[i].treeEdges = 0;
				globalStatus = PEELING;
                totalSyncs++;
			}
        	break;
        case GROWING:
        	msg.TYPE = NEWINFO;
            totalSyncs += graphStatus[i].borders_size * vertex_connection_count(1); //here the input to vertex_connection_count is trivial to the computation
            msg.info.borders = graphStatus[i].borders;
			inputStream.write(msg);
            break;
        default:
            break;
        }

    }
    status.write(globalStatus);
    n_expansions.write(totalSyncs);
    n_fusions.write(totalSyncs);
}

void mainLoop(hls::stream<Edge>& correctionEdges,
              PU graphStatusCPY[SYN_LEN],
              PU graphStatus[SYN_LEN],
              bool& oddsR,
              bool& oddsW,
              bool& doneW
              )
{
    hls_thread_local hls::task GrowingUnit[GROWUNITCOUNT];
#pragma HLS ARRAY_PARTITION variable=GrowingUnit type=complete
    hls_thread_local hls::task PeelingUnit[GROWUNITCOUNT];
#pragma HLS ARRAY_PARTITION variable=PeelingUnit type=complete
    hls_thread_local hls::split::round_robin<Message, GROWUNITCOUNT, CORR_LEN>inputChannel;
    hls_thread_local hls::merge::load_balance<Edge, GROWUNITCOUNT, CORR_LEN * 4> growEdges;
	hls_thread_local hls::merge::load_balance<ap_uint<BITSACCURACY>, PEELUNITCOUNT, CORR_LEN> outputChannel;

	hls_thread_local hls::split::round_robin<Tree, PEELUNITCOUNT, CORR_LEN> trees;
	hls_thread_local hls::merge::load_balance<Edge, PEELUNITCOUNT, CORR_LEN> corrEdges;
    hls::stream<Edge> fuseEdges;
#pragma HLS STREAM variable=fuseEdges depth=CORR_LEN
    hls::stream<bool> done;
    hls::stream<ap_uint<BITSACCURACY>> status;
    hls::stream<ap_uint<BITSACCURACY>> totalSyncsChannel;
    hls::stream<ap_uint<BITSACCURACY>> totalPossibleFusions;


    ///DATAFLOW no cerchi

#pragma HLS DATAFLOW

    sendInfo(inputChannel.in, status, totalSyncsChannel, totalPossibleFusions, graphStatusCPY, oddsR, trees.in);
#ifdef CBUILD
    static bool started = false;
    if(!started)
    {
#endif
    for(int i = 0; i < GROWUNITCOUNT; i++)
    {
#pragma HLS UNROLL
    	GrowingUnit[i](FSMProcessGrow, inputChannel.out[i], growEdges.in[i]);
    }

    for(int i = 0; i < PEELUNITCOUNT; i++)
    {
#pragma HLS UNROLL
        PeelingUnit[i](FSMProcessPeel, trees.out[i], corrEdges.in[i], outputChannel.in[i]);
    }
#ifdef CBUILD
    started = true;
    }
#endif

    syncPU(fuseEdges, status, outputChannel.out, done, growEdges.out, corrEdges.out, correctionEdges, totalSyncsChannel, doneW);
    fusion(fuseEdges, done, totalPossibleFusions, graphStatus, oddsW);
}


void controllerProcess(hls::stream<PU>& infoPE, hls::stream<Edge>& correctionEdges)
{
    bool done = false;
    bool doneCPY = false;

    bool odds = true;
    bool oddsCPY = true;

    static PU graphStatus[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=graphStatus type=complete dim=2

    static PU graphStatusCPY[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=graphStatusCPY type=complete dim=2


    for(int i = 0; i < SYN_LEN; i++)
    {
    	graphStatus[i].status = TRIVIAL;
    }

    //cluster initialization starts
    //giving usefulInfo only to the working ones
    TRIVIAL_PU:
    do
    {
        PU info = infoPE.read();
        info.treeEdges = 0;
        graphStatus[info.ID] = info;
    }while(!infoPE.empty());

    //filling the remaining with garbage info
    NONTRIVIAL_PU:
    for(int i = 0; i < SYN_LEN; i++)
    {
        if(graphStatus[i].status == TRIVIAL)
        {
            PU info{};
            info.ID = i;
			info.parity = 0;
			info.borders = 0;
            info.borders[INDEX(i)] = 1;
            info.borders_size++;
            info.nodes_to_peel = 0;
            info.treeEdges = 0;
            graphStatus[i] = info;
        }
    }
    //cluster initialization ends

    //Sync and fusion loop
    MAIN_LOOP:
	while(!done)
    {

    	DOUBLECOPY:
		for (int j = 0; j < SYN_LEN; j++)
		{
#pragma HLS UNROLL
			graphStatusCPY[j].ID = graphStatus[j].ID;
            graphStatusCPY[j].status = graphStatus[j].status;
            graphStatusCPY[j].parity = graphStatus[j].parity;
            graphStatusCPY[j].nodes_to_peel = graphStatus[j].nodes_to_peel;
            graphStatusCPY[j].borders = graphStatus[j].borders;
            graphStatusCPY[j].borders_size = graphStatus[j].borders_size;
            graphStatusCPY[j].treeEdges = graphStatus[j].treeEdges;
		}
        //if(!done)
        //{
            mainLoop(correctionEdges, graphStatusCPY, graphStatus, odds, oddsCPY, doneCPY);
        //}
		odds = oddsCPY;
		done = doneCPY;
    }
}



void syncPU(hls::stream<Edge>& fuseEdges,
            hls::stream<ap_uint<BITSACCURACY>>& status,
            hls::stream<ap_uint<BITSACCURACY>>& outputChannel,
            hls::stream<bool>& done,
            hls::stream<Edge>& growEdges,
			hls::stream<Edge>& corrEdges,
            hls::stream<Edge>& correctionEdges,
            hls::stream<ap_uint<BITSACCURACY>>& totalSyncsChannel,
            bool& doneW)
{

    static ap_uint<BITSACCURACY> support[CORR_LEN] = {0};
#pragma HLS ARRAY_PARTITION variable=support type=complete

    ap_uint<BITSACCURACY> stat = status.read();
    ap_uint<BITSACCURACY> totalSyncs = totalSyncsChannel.read();
    switch(stat)
    {
        case GROWING:
            done.write(false);
            doneW = false;
        READING_EXPANSIONS:
            for(int i = 0; i < totalSyncs; i++)
            {
                Edge e = growEdges.read();
                uint8_t id = edge_idx(e);
                ap_uint<BITSACCURACY> expValue = support[id];
                if(expValue != 3)
                {
                    expValue = std::min(static_cast<int>(expValue + 1), 2);
                    if(expValue == 2)
                    {
                        fuseEdges.write(e);
                        expValue = 3;
                    }
                    else
                    {
                    	fuseEdges.write({255, 255});
                    }
                }
                else
                {
                	fuseEdges.write({255, 255});
                }
                support[id] = expValue;
            }
            break;
        case PEELING:
            done.write(true);
            doneW = true;
            ap_uint<BITSACCURACY> counter = 0;
            SYNC_PU:
            for(int i = 0; i < totalSyncs; i++)
            {
                counter += outputChannel.read();
            }
            READING_CORRECTIONS:
            for(int i = 0; i < counter; i++) {
                Edge e = corrEdges.read();
                correctionEdges.write(e);
            }

            RESET_SUPPORT:
            for(int i = 0; i < CORR_LEN; i++)
            {
#pragma HLS UNROLL
                support[i] = 0;
            }
            break;
    }

}

void pack(Vector<ap_uint<BITSACCURACY>, SYN_LEN> borders, const int packIndexes[SYN_LEN])
{
    int scanResult[SYN_LEN] = {0};
#pragma HLS ARRAY_PARTITION variable=scanResult type=complete
    int acc = 0;
    PACK_ACC_LOOP:
    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS PIPELINE II=1
        scanResult[i] = acc;
        acc = acc + packIndexes[i];
    }
    PACK_PACKING_LOOP:
    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL
        borders.array[scanResult[i]] = borders.at(i);
    }

    borders.size = acc;
}


void bordersFusion(PU graphStatus[SYN_LEN],
                   ap_uint<BITSACCURACY> root_of_vertex[SYN_LEN],
                   ap_uint<BITSACCURACY> connection_counts[SYN_LEN],
                   int root1, int root2)
{
    graphStatus[root1].borders = graphStatus[root1].borders ^ graphStatus[root2].borders;
    graphStatus[root1].borders_size = 0;


    BORDER_FUSION:
    for (uint8_t i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL
        if(graphStatus[root2].borders[INDEX(i)])
        {
            ap_uint<BITSACCURACY> vertex = i;
            graphStatus[vertex].status = FUSED;
            root_of_vertex[vertex] = root1;
        }
        if(graphStatus[root1].borders[INDEX(i)])
        {
            if(connection_counts[i] < 4)
            {
                graphStatus[root1].borders_size++;
            }
        	else
            {
                graphStatus[root1].borders[INDEX(i)] = 0;
            }
        }
    }
}

void treeFusion(PU graphStatus[SYN_LEN],
                int root1, int root2)
{
    graphStatus[root1].treeEdges = graphStatus[root1].treeEdges | graphStatus[root2].treeEdges;

    graphStatus[root1].nodes_to_peel += graphStatus[root2].nodes_to_peel;
    graphStatus[root2].treeEdges = 0;
}


void fusion_internal(PU graphStatus[SYN_LEN],
                     ap_uint<BITSACCURACY> root_of_vertex[SYN_LEN],
                     ap_uint<BITSACCURACY> connection_counts[SYN_LEN],
                     int root1, int root2)
{
#pragma HLS DATAFLOW
    bordersFusion(graphStatus, root_of_vertex, connection_counts, root1, root2);
    treeFusion(graphStatus, root1, root2);
}

void pushEdgeToTree(ap_uint<SYN_LEN * 4>& treeEdges, Edge e)
{
    if(!is_horizontal(e))
    {
        ap_uint<BITSACCURACY> lowerV = lower(e);
        ap_uint<BITSACCURACY> upperV = (lowerV == e.u) ? e.v : e.u;

        //every vertex can have connections to N,S,W,E. This information is stored in quartets of bits where each bit represents if the connection exists
        treeEdges[lowerV * 4 + 0] = 1;
        treeEdges[upperV * 4 + 1] = 1;
    }
    else
    {
        ap_uint<BITSACCURACY> leftV = left(e);
        ap_uint<BITSACCURACY> rightV = (leftV == e.u) ? e.v : e.u;

        treeEdges[leftV * 4 + 3] = 1;
        treeEdges[rightV * 4 + 2] = 1;
    }
}


void fusion(hls::stream<Edge>& fuseEdges,
            hls::stream<bool>& done,
            hls::stream<ap_uint<BITSACCURACY>>& n_fusions,
            PU graphStatus[SYN_LEN],
            bool& odds)
{

    static bool initialized = false;
    static ap_uint<BITSACCURACY> root_of_vertex[SYN_LEN] = {0};
#pragma HLS ARRAY_PARTITION variable=root_of_vertex type=complete
    static ap_uint<BITSACCURACY> connection_counts[SYN_LEN] = {0};
#pragma HLS ARRAY_PARTITION variable=connection_counts type=complete
    static bool involvedInFusion[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=involvedInFusion type=complete

    if(!initialized)
    {
        INITIALIZE_FUSION_DATA:
        for(int i = 0; i < SYN_LEN; i++)
        {
#pragma HLS UNROLL
            root_of_vertex[i] = i;
            connection_counts[i] = 0;
        }
        initialized = true;
    }

    ap_uint<BITSACCURACY> totalFusions = n_fusions.read();

    bool finish = done.read();
    if(!finish)
    {
        odds = false;
        int counter = 0;
        hls::stream<Edge> EdgeToFuse("EDGES");
#pragma HLS STREAM variable=EdgeToFuse depth=CORR_LEN

        PREPROCESS:
        for(int j = 0; j < totalFusions; j++)
        {
            Edge e = fuseEdges.read();
            if(!(e.u == 255 && e.v == 255))
            {
                connection_counts[e.u]++;
                connection_counts[e.v]++;
                counter++;
                EdgeToFuse.write(e);
            }
        }


        FUSION:
        for(int i = 0; i < counter; i++)
        {
#pragma HLS DEPENDENCE variable=graphStatus type=inter dependent=false
#pragma HLS DEPENDENCE variable=involvedInFusion type=inter dependent=false
            Edge e = EdgeToFuse.read();

            ap_uint<BITSACCURACY> root1 = root_of_vertex[e.u];
            ap_uint<BITSACCURACY> root2 = root_of_vertex[e.v];

            if(root1 != root2)
            {
                while(involvedInFusion[root1] || involvedInFusion[root2]);

                involvedInFusion[root1] = true;
                involvedInFusion[root2] = true;

                if(graphStatus[root1].status == TRIVIAL)
                {
                    ap_uint<BITSACCURACY> tmp = root1;
                    root1 = root2;
                    root2 = tmp;
                }

                fusion_internal(graphStatus, root_of_vertex, connection_counts, root1, root2);

                if (!(graphStatus[root1].parity == 0 || graphStatus[root2].parity == 0)) {
                    graphStatus[root1].parity += graphStatus[root2].parity;

                    if (graphStatus[root1].parity % 2 == 0) {
                        graphStatus[root1].status = WAITING;
                    } else {
                        graphStatus[root1].status = GROWING;
                    }
                }


                pushEdgeToTree(graphStatus[root1].treeEdges, e);
                graphStatus[root1].nodes_to_peel++;

                involvedInFusion[root1] = false;
                involvedInFusion[root2] = false;
            }

        }

        ODD_CHECK:
        for(int i = 0; i < SYN_LEN; i++)
        {
            if(graphStatus[i].status == GROWING)
            {
                odds = true;
            }
        }

        if(!odds)
        {
            initialized = false;
        }
    }
}


