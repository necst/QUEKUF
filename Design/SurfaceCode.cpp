#include "SurfaceCode.h"


ap_uint<BITSACCURACY> to_vertex_index(int row, int col)
{
    if(row < 0)
    {
        row += L;
    }
    else if(row >= L)
    {
        row -= L;
    }
    if(col < 0)
    {
        col += L;
    }
    else if(col >= L)
    {
        col -= L;
    }


    return L * row + col;
}

ap_uint<BITSACCURACY> vertex_connection_count(ap_uint<BITSACCURACY> vertex)
{
    return 4;
}

Vector<ap_uint<BITSACCURACY>,5> vertex_connections(ap_uint<BITSACCURACY> v)
{
    int row = v/L;
    int col = v%L;

    Vector<ap_uint<BITSACCURACY>, 5> vector;
#pragma HLS ARRAY_PARTITION variable=vector.array type=complete

    ap_uint<BITSACCURACY> idx1 = to_vertex_index(row - 1, col);
    vector.pushIn(idx1);
    ap_uint<BITSACCURACY> idx2 = to_vertex_index(row + 1, col);
    vector.pushIn(idx2);
    ap_uint<BITSACCURACY> idx3 = to_vertex_index(row, col - 1);
    vector.pushIn(idx3);
    ap_uint<BITSACCURACY> idx4 = to_vertex_index(row, col + 1);
    vector.pushIn(idx4);

    return vector;
}

Coord vertex_to_coord(const uint8_t vidx)
{
    return {vidx / L, vidx % L};
}

bool is_horizontal(Edge e)
{
    return ((e.v - e.u) == 1) || ((e.v - e.u) == (L - 1));
}

uint8_t left(Edge e)
{
    if((e.v - e.u) == 1)
    {
        return e.u;
    }
    return e.v;
}

uint8_t lower(Edge e)
{
    if((e.v - e.u) == L)
    {
        return e.v;
    }
    return e.u;
}

ap_uint<BITSACCURACY> edge_idx(Edge e)
{
    if(is_horizontal(e))
    {
        Coord tmp=vertex_to_coord(left(e));
        return L*(L+tmp.x)+tmp.y;
    }
    else
    {
        return lower(e);
    }
}

Edge idToEdge(int id)
{
    if(id >= SYN_LEN)
    {
        uint8_t col = id%D;
        uint8_t row = id/D - D;

        uint8_t colNext = (col+1)%D;
        uint8_t id1= static_cast<uint8_t>(row * D+col);
        uint8_t id2= static_cast<uint8_t>(row*D+colNext);

        return {std::min(id1, id2),std::max(id1,id2)};
    }
    else
    {
        uint8_t col = id%D;
        int row = id/D;

        int rowNext = (row-1);
        if(rowNext < 0)
        {
            rowNext = 2;
        }

        uint8_t id1= static_cast<uint8_t>(row * D+col);
        uint8_t id2= static_cast<uint8_t>(rowNext*D+col);

        return {std::min(id1, id2),std::max(id1,id2)};

    }
}
