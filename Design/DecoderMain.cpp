#include "DecoderMain.h"
#include "Controller.h"


void perfCounterProc(hls::stream<int64_t>& cmd, int64_t* out) {
    int64_t val;
    // wait to receive a value to start counting
    int64_t cnt = cmd.read();
// keep counting until a value is available
    count:
    while (cmd.read_nb(val) == false) {
        cnt++;
    }

    *out = cnt;
}

void mainBody(hls::stream<PU>& infoPU,
              hls::stream<Edge>& correctionEdges,
              hls::stream<int64_t>& cmd)
{
    cmd.write(1);
    controllerProcess(infoPU, correctionEdges);
    cmd.write(1);
}


void counterDF(hls::stream<PU>& infoPU,
               hls::stream<Edge>& correctionEdges,
               int64_t* cc)
{
    hls::stream<int64_t> cmdForCounter;
#pragma HLS DATAFLOW
    mainBody(infoPU, correctionEdges, cmdForCounter);
    perfCounterProc(cmdForCounter, cc);
}



void decoderTop(bool syndrome[SYN_LEN], bool correction[CORR_LEN], int64_t* totalClocks)
{
#pragma HLS INTERFACE m_axi port=syndrome offset=slave bundle=gmem0 depth=SYN_LEN
#pragma HLS INTERFACE m_axi port=correction offset=slave bundle=gmem1 depth=CORR_LEN
#pragma HLS INTERFACE m_axi port=totalClocks offset=slave bundle=gmem2 depth=2

#pragma HLS INTERFACE s_axilite port=syndrome bundle=control
#pragma HLS INTERFACE s_axilite port=correction bundle=control
#pragma HLS INTERFACE s_axilite port=totalClocks bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    bool CorrectionCPY[CORR_LEN] = {0};


    hls::stream<bool> syndrome_cpy;
#pragma HLS STREAM variable=syndrome_cpy depth=CORR_LEN
    hls::stream<ap_uint<BITSACCURACY>> synBits("SyndromeErrors");
#pragma HLS STREAM variable=synBits depth=SYN_LEN
    hls::stream<PU> infoPU("ProcessingUnitInfo");
#pragma HLS STREAM variable=infoPU depth=SYN_LEN
    hls::stream<Edge> correctionEdges("CorrectionPrediction");
#pragma HLS STREAM variable=correctionEdges depth=CORR_LEN


    readSyn(syndrome, synBits, syndrome_cpy);
    initPU(synBits, infoPU, syndrome_cpy);
    counterDF(infoPU, correctionEdges, totalClocks);
    translation(correctionEdges, CorrectionCPY);
    for(int i = 0; i < CORR_LEN; i++)
    {
        correction[i] = CorrectionCPY[i];
    }
}


void readSyn(bool syndrome[SYN_LEN], hls::stream<ap_uint<BITSACCURACY>>& SynBits, hls::stream<bool>& syndrome_cpy)
{
    READ_SYN_BITS:
    for(int i = 0; i < SYN_LEN; i++)
    {
        if(syndrome[i] != 0)
        {
            SynBits.write(i);
        }
    }
    READ_SYN:
    for(int i = 0; i < SYN_LEN; i++)
	{
		syndrome_cpy.write(syndrome[i]);
	}
}

void initPU(hls::stream<ap_uint<BITSACCURACY>>& SynBits, hls::stream<PU>& infoPU, hls::stream<bool>& syndrome_cpy)
{
    bool syn_cpy[SYN_LEN];
#pragma HLS ARRAY_PARTITION variable=syn_cpy type=complete
    SYN_CPY:
    for(int i = 0; i < SYN_LEN; i++)
    {
        syn_cpy[i] = syndrome_cpy.read();
    }

    INIT:
    do
    {
        PU info{};
#pragma HLS ARRAY_PARTITION variable=info.borders.array type=complete
#pragma HLS ARRAY_PARTITION variable=info.syn_CPY type=complete
        info.ID = SynBits.read();
        info.parity = 1;
        info.status = GROWING;
        info.borders.pushIn(info.ID);
        for(int i = 0; i < SYN_LEN; i++)
        {
#pragma HLS UNROLL
            info.syn_CPY[i] = syn_cpy[i];
        }

        infoPU.write(info);
    }while(!SynBits.empty());
}

void translation(hls::stream<Edge>& correctionEdges, bool correction[CORR_LEN])
{
    for(int i = 0; i < CORR_LEN; i++)
    {
        correction[i] = 0;
    }
	TRANSLATION:
    do
    {
        Edge e = correctionEdges.read();
        Edge t = e;

        e.u = std::min(t.u, t.v);
        e.v = std::max(t.u, t.v);
        correction[edge_idx(e)] = true;
    }while(!correctionEdges.empty());
}
