#include "Controller.h"
#include "ClusterUnit.h"
#include "Defines.h"
#include "hls_print.h"

#define CBUILD

void sendInfo(hls::stream<Message> inputStream[SYN_LEN],
              hls::stream<ap_uint<BITSACCURACY>>& status,
              hls::stream<ap_uint<BITSACCURACY>>& n_expansions,
			  hls::stream<ap_uint<BITSACCURACY>>& n_fusions,
              PU graphStatus[SYN_LEN],
              bool& odds,
              hls::stream<Edge> treeEdges[SYN_LEN])
{
    ap_uint<BITSACCURACY> globalStatus = GROWING;
    ap_uint<BITSACCURACY> totalSyncs = 0;
WRITING_TO_PU:
    for(int i = 0; i < SYN_LEN; i++)
    {
        Message msg{};
#pragma HLS ARRAY_PARTITION variable=msg.info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=msg.info.syn_CPY type=complete
        switch(graphStatus[i].status)
        {
        case WAITING:
        	if(odds == 0)
			{
				msg.TYPE = STARTPEEL;
				PREPARING_TREE_EDGES:
				for(int j = 0; j < graphStatus[i].treeEdges.getSize(); j++)
				{
					treeEdges[i].write(graphStatus[i].treeEdges.at(j));
				}
                graphStatus[i].treeEdges.reset();
				globalStatus = PEELING;
                totalSyncs++;
			}
        	else
        	{
        		msg.TYPE = NEWINFOWAITING;
        	}
        	break;
        case FUSED:
        	msg.TYPE = NEWINFOWAITING;
        	break;
        case TRIVIAL:
        	msg.TYPE = TRIVIALNFO;
        	break;
        case GROWING:
        	msg.TYPE = NEWINFO;
            totalSyncs += graphStatus[i].borders.size * vertex_connection_count(1); //here the input to vertex_connection_count is non-trivial to the computation
        	break;
        default:
            break;

        }


        msg.info.nodes_to_peel = graphStatus[i].nodes_to_peel;
        msg.info.borders = graphStatus[i].borders;
        SYN_CPY_CPY:
        for(int j = 0; j < SYN_LEN; j++)
        {
#pragma HLS UNROLL
            msg.info.syn_CPY[j] = graphStatus[i].syn_CPY[j];
        }
        inputStream[i].write(msg);
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
    hls_thread_local hls::task ProcessingUnit[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=ProcessingUnit type=complete
    hls_thread_local hls::stream<Message> inputChannel[SYN_LEN];
#pragma HLS STREAM variable=inputChannel depth = SYN_LEN
	hls_thread_local hls::merge::load_balance<bool, SYN_LEN, CORR_LEN> outputChannel;
	hls_thread_local hls::stream<Edge> treeEdges[SYN_LEN];
#pragma HLS STREAM variable=treeEdges depth=CORR_LEN
	hls_thread_local hls::merge::load_balance<Edge, SYN_LEN, CORR_LEN*4> growEdges;
	hls_thread_local hls::merge::load_balance<Edge, SYN_LEN, CORR_LEN> corrEdges;
    hls::stream<Edge> fuseEdges;
#pragma HLS STREAM variable=fuseEdges depth=CORR_LEN
    hls::stream<bool> done;
    hls::stream<ap_uint<BITSACCURACY>> status;
    hls::stream<ap_uint<BITSACCURACY>> totalSyncsChannel;
    hls::stream<ap_uint<BITSACCURACY>> totalPossibleFusions;


    ///DATAFLOW no cerchi

#pragma HLS DATAFLOW

    sendInfo(inputChannel, status, totalSyncsChannel, totalPossibleFusions, graphStatusCPY, oddsR, treeEdges);
#ifdef CBUILD
    static bool started = false;
    if(!started)
    {
#endif
    for(int i = 0; i < SYN_LEN; i++)
    {
#pragma HLS UNROLL
    	ProcessingUnit[i](FSMProcess, inputChannel[i], treeEdges[i], growEdges.in[i], corrEdges.in[i], outputChannel.in[i]);
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
#pragma HLS ARRAY_PARTITION variable=info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=info.syn_CPY type=complete
        info.treeEdges.reset();
        graphStatus[info.ID] = info;
    }while(!infoPE.empty());

    //filling the remaining with garbage info
    NONTRIVIAL_PU:
    for(int i = 0; i < SYN_LEN; i++)
    {
        if(graphStatus[i].status == TRIVIAL)
        {
            PU info{};
#pragma HLS ARRAY_PARTITION variable=info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=info.syn_CPY type=complete
            info.ID = i;
			info.parity = 0;
            info.borders.pushIn(i);
            info.nodes_to_peel = 0;
            info.treeEdges.reset();
            graphStatus[i] = info;
        }
    }
    //cluster initialization ends

    //Sync and fusion loop
    MAIN_LOOP:
    //for(int i = 0; i < CORR_LEN; i++)
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
            graphStatusCPY[j].treeEdges = graphStatus[j].treeEdges;
            SYN_CPY_CPY:
            for(int i = 0; i < SYN_LEN; i++)
            {
#pragma HLS UNROLL
                graphStatusCPY[j].syn_CPY[i] = graphStatus[j].syn_CPY[i];
            }
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
            hls::stream<bool>& outputChannel,
            hls::stream<bool>& done,
            hls::stream<Edge>& growEdges,
			hls::stream<Edge>& corrEdges,
            hls::stream<Edge>& correctionEdges,
            hls::stream<ap_uint<BITSACCURACY>>& totalSyncsChannel,
            bool& doneW)
{

    static ap_uint<BITSACCURACY> support[CORR_LEN] = {0};

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
            SYNC_PU:
            for(int i = 0; i < totalSyncs; i++)
            {
                outputChannel.read();
            }
            READING_CORRECTIONS:
            do
            {
                Edge e = corrEdges.read();
                correctionEdges.write(e);
            }while(!corrEdges.empty());

            RESET_SUPPORT:
            for(int i = 0; i < CORR_LEN; i++)
            {
#pragma HLS UNROLL
                support[i] = 0;
            }
            break;
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
    static ap_uint<BITSACCURACY> connection_counts[SYN_LEN] = {0};
    static Vector<ap_uint<BITSACCURACY>> vertices[SYN_LEN];

    if(!initialized)
    {
        INITIALIZE_FUSION_DATA:
        for(int i = 0; i < SYN_LEN; i++)
        {
#pragma HLS UNROLL
            root_of_vertex[i] = i;
            connection_counts[i] = 0;
            vertices[i].reset();
            vertices[i].pushIn(i);
        }
        initialized = true;
    }

    ap_uint<BITSACCURACY> totalFusions = n_fusions.read();

    bool finish = done.read();
    if(!finish)
    {
        odds = false;

        FUSION:
        for(int j = 0; j < totalFusions; j++)
        {
            Edge e = fuseEdges.read();

            if(!(e.u == 255 && e.v == 255))
            {
                connection_counts[e.u]++;
                connection_counts[e.v]++;

                ap_uint<BITSACCURACY> root1 = root_of_vertex[e.u];
                ap_uint<BITSACCURACY> root2 = root_of_vertex[e.v];

                if(graphStatus[root1].status == TRIVIAL)
                {
                    ap_uint<BITSACCURACY> tmp = root1;
                    root1 = root2;
                    root2 = tmp;
                }

                if(root1 != root2) 
				{
					graphStatus[root1].borders.reset();
                    BORDER_FUSION:
                    for (uint8_t i = 0; i < vertices[root1].getSize(); i++) {
                        ap_uint<BITSACCURACY> vertex = vertices[root1].at(i);
                        if (connection_counts[vertex] < 4) {
                             graphStatus[root1].borders.pushIn(vertex);
                        }
                    }
					
                    VERTEX_FUSION:
                    for (uint8_t i = 0; i < vertices[root2].getSize(); i++)
                    {
                        ap_uint<BITSACCURACY> vertex = vertices[root2].at(i);
						graphStatus[vertex].status = FUSED;
                        root_of_vertex[vertex] = root1;
                        vertices[root1].pushIn(vertex);
                        if (connection_counts[vertex] < 4)
                        {
                            graphStatus[root1].borders.pushIn(vertex);
                        }
					}

                    TREE_FUSION:
                    for (uint8_t i = 0; i < graphStatus[root2].treeEdges.getSize(); i++)
                    {
                        graphStatus[root1].treeEdges.pushIn(graphStatus[root2].treeEdges.at(i));
                        graphStatus[root1].nodes_to_peel++;
                    }
					graphStatus[root2].treeEdges.reset();

					if(!(graphStatus[root1].parity == 0 || graphStatus[root2].parity == 0))
					{
                        graphStatus[root1].parity += graphStatus[root2].parity;

						if (graphStatus[root1].parity % 2 == 0) 
						{
							graphStatus[root1].status = WAITING;
						} 
						else 
						{
							graphStatus[root1].status = GROWING;
						}
					}
                    

                    graphStatus[root1].treeEdges.pushIn(e);
                    graphStatus[root1].nodes_to_peel++;      
                }
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
