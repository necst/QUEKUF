#ifndef QEC_UNION_FIND_DEFINES_H
#define QEC_UNION_FIND_DEFINES_H

#define K 2
#define D 8
#define BITSACCURACY 8
#define L D
#define N D*D*2
#define CORR_LEN D*D*2
#define SYN_LEN D*D

#include "hls_task.h"
#include "hls_np_channel.h"
#include "ap_int.h"
#include "Vector.h"
#include "SurfaceCode.h"


enum STATUS
{
    GROWING,
    WAITING,
    TRIVIAL,
    FUSED,
    PEELING,
    FINISHED
};

enum MESSAGE_TO_PU
{
    NEWINFO,
    TRIVIALNFO,
	NEWINFOWAITING,
    STOP_EVERYTHING,
    STARTPEEL
};

enum MESSAGE_TO_CONTROLLER
{
    PEELED,
    GREW,
    OFFLINE
};

enum SYNC_STAT
{
    NEED_TO_SYNC,
    SYNCED
};

struct TreeNode
{
    Vector<ap_uint<BITSACCURACY>, 5> children;
    ap_uint<BITSACCURACY> totalConnections = 0;
};


struct PU
{
    ap_uint<BITSACCURACY> ID = 0;
    ap_uint<BITSACCURACY> status = TRIVIAL;
    ap_uint<BITSACCURACY> parity = 0;
    ap_uint<BITSACCURACY> nodes_to_peel = 0;
    Vector<ap_uint<BITSACCURACY>> borders;
    Vector<Edge> treeEdges;
    bool syn_CPY[SYN_LEN];
};

struct PUtoSend
{
    ap_uint<BITSACCURACY> nodes_to_peel = 0;
    Vector<ap_uint<BITSACCURACY>> borders;
    bool syn_CPY[SYN_LEN];
};

struct Message
{
    ap_uint<BITSACCURACY> TYPE;
    PUtoSend info{};
};



#endif //QEC_UNION_FIND_DEFINES_H
