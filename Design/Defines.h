#ifndef QEC_UNION_FIND_DEFINES_H
#define QEC_UNION_FIND_DEFINES_H

#define K 2
#define D 3
#define BITSACCURACY 8
#define L D
#define N D*D*2
#define CORR_LEN D*D*2
#define SYN_LEN D*D
#define GROWUNITCOUNT 4
#define PEELUNITCOUNT 2

#define INDEX(v) (SYN_LEN - 1 - v)

#include "hls_task.h"
#include "hls_np_channel.h"
#include "ap_int.h"
#include "Vector.h"
#include "ToricCode.h"


enum STATUS
{
    GROWING,
    WAITING,
    TRIVIAL,
    FUSED,
    PEELING,
};

enum MESSAGE_TO_PU
{
    NEWINFO,
    TRIVIALNFO,
	NEWINFOWAITING,
    STARTPEEL
};

struct Tree
{
    ap_uint<SYN_LEN * 4> treeEdges;
    ap_uint<BITSACCURACY> nodes_to_peel = 0;
    bool syn_CPY[SYN_LEN];
};


struct PU
{
    ap_uint<BITSACCURACY> ID = 0;
    ap_uint<BITSACCURACY> status = TRIVIAL;
    ap_uint<BITSACCURACY> parity = 0;
    ap_uint<BITSACCURACY> nodes_to_peel = 0;
    ap_uint<SYN_LEN> borders;
    ap_uint<BITSACCURACY> borders_size = 0;
    ap_uint<SYN_LEN * 4> treeEdges;
};

struct PUtoSend
{
    ap_uint<SYN_LEN> borders;
};

struct Message
{
    ap_uint<BITSACCURACY> TYPE;
    PUtoSend info{};
};

extern bool globalSyndrome[SYN_LEN];



#endif //QEC_UNION_FIND_DEFINES_H
