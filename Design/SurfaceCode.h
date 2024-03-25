#include "Defines.h"

#ifndef QEC_UNION_FIND_SURFACECODE_H
#define QEC_UNION_FIND_SURFACECODE_H

struct Edge
{
    ap_uint<BITSACCURACY> u;
    ap_uint<BITSACCURACY> v;

    bool operator==(const Edge& other) const
    {
        return (u == other.u && v == other.v);
    }
};

struct Coord
{
    ap_uint<8> x;
    ap_uint<8> y;
};

ap_uint<BITSACCURACY> to_vertex_index(int row, int col);

ap_uint<BITSACCURACY> vertex_connection_count(ap_uint<BITSACCURACY> vertex);

Vector<ap_uint<BITSACCURACY>,5> vertex_connections(ap_uint<BITSACCURACY> v);

ap_uint<BITSACCURACY> num_vertices();

Coord vertex_to_coord(const ap_uint<BITSACCURACY> vidx);

ap_uint<BITSACCURACY> num_edges();

bool is_horizontal(Edge e);

uint8_t left(Edge e);

uint8_t lower(Edge e);

ap_uint<BITSACCURACY> edge_idx(Edge e);

Edge idToEdge(uint8_t id);

#endif //QEC_UNION_FIND_SURFACECODE_H
