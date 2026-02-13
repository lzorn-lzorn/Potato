#pragma once
#include <limits>
#include <vector>
#include <cassert>

#include "../../Math/Vector.h"



namespace Core{
using Vector3Df   = Vector3D<float>;
using Vector2Df   = Vector2D<float>;
using VertexIndex = uint32_t;
using EdgeIndex   = uint32_t;
using FaceIndex   = uint32_t;

constexpr VertexIndex INVALID_VERTEX = std::numeric_limits<VertexIndex>::max();
constexpr EdgeIndex   INVALID_EDGE   = std::numeric_limits<EdgeIndex>::max();
constexpr FaceIndex   INVALID_FACE   = std::numeric_limits<FaceIndex>::max();

template<typename Ty>
class MemoryPool {
public:
    static constexpr size_t chunk_size = 512; // 每块512个元素
    
    MemoryPool() { allocate_chunk(); }
    
    VertexIndex allocate() {
        if (free_list.empty()) {
            allocate_chunk();
        }
        VertexIndex idx = free_list.back();
        free_list.pop_back();
        return idx;
    }
    
    void deallocate(VertexIndex idx) {
        free_list.push_back(idx);
    }
    
    Ty& operator[](VertexIndex idx) { return data[idx]; }
    const Ty& operator[](VertexIndex idx) const { return data[idx]; }
    
    size_t size() const { return data.size(); }
    size_t capacity() const { return data.capacity(); }
    
private:
    void allocate_chunk() {
        size_t old_size = data.size();
        data.resize(old_size + chunk_size);
        for (size_t i = 0; i < chunk_size; ++i) {
            free_list.push_back(static_cast<VertexIndex>(old_size + i));
        }
    }
    
    std::vector<Ty> data;
    std::vector<VertexIndex> free_list;
};


struct alignas(64) Vertex {
	Vector3Df position;        // 12Bytes
	int32_t _m_padding_0;     // 4Bytes, 用于对齐
	Vector3Df normal;  
	int32_t _m_padding_1;     // 4Bytes, 用于对齐
	
	EdgeIndex outgoing_edge;  // 4Bytes, 指向以该顶点为起点的某条边
	uint32_t flags;           // 4Bytes, 用于存储顶点属性（如是否选中、是否边界等）
	uint16_t edge_count;      // 2Bytes, 以该顶点为起点的边的数量
	uint16_t _m_padding_2;    // 2Bytes, 用于对齐
	uint32_t custom_data_idx; // 4Bytes, 用于关联外部数据（如纹理坐标、颜色等）

	float reserved[4];        // 16Bytes, 预留空间，未来可用于存储更多属性或数据

	Vertex() : 
		position{Vector3Df::ZeroVector()}, 
		normal{Vector3Df::ZAxisVector()},
	    outgoing_edge(INVALID_EDGE), 
		flags(0), 
		edge_count(0), 
		custom_data_idx(0) 
	{}

	bool IsValid() const noexcept { return outgoing_edge != INVALID_EDGE; }
    bool IsBoundary() const noexcept { return flags & 0x01; }
};

struct alignas(64) HalfEdge {
    // ===== 热数据区（前32Bytes）=====
    VertexIndex vertex;      // 4Bytes: 目标顶点
    EdgeIndex twin;          // 4Bytes: 对边索引
    EdgeIndex next;          // 4Bytes: 同面下一边
    EdgeIndex prev;          // 4Bytes: 同面前一边
    FaceIndex face;          // 4Bytes: 所属面
    uint32_t flags;          // 4Bytes: 标志位
    
    // ===== 径向循环（中间16Bytes）=====
    EdgeIndex radial_next;   // 4Bytes: 径向下一边（同边不同面）
    EdgeIndex radial_prev;   // 4Bytes: 径向前一边
    uint16_t radial_count;   // 2Bytes: 共享该边的面数（通常2）
    uint16_t _m_padding_0;   // 2Bytes对齐
    uint32_t custom_data_id; // 4Bytes: 冷数据索引
    
    // ===== 预留区（最后16Bytes）=====
    float _reserved[4];
    
    HalfEdge() : 
		vertex(INVALID_VERTEX), 
		twin(INVALID_EDGE),
		next(INVALID_EDGE), 
		prev(INVALID_EDGE),
		face(INVALID_FACE), 
		flags(0),
		radial_next(INVALID_EDGE), 
		radial_prev(INVALID_EDGE),
		radial_count(0), 
		custom_data_id(UINT32_MAX) {}

    bool IsValid() const noexcept { return vertex != INVALID_VERTEX; }
    bool IsBoundary() const noexcept { return face == INVALID_FACE; }
};

struct alignas(32) Face {
    EdgeIndex first_edge;    // 4Bytes: 第一条半边
    Vector3Df normal;        // 12Bytes: 面法线
    uint16_t edge_count;     // 2Bytes: 边数（3=三角形，4=四边形）
    uint16_t material_id;    // 2Bytes: 材质ID
    uint32_t flags;          // 4Bytes: 标志位
    uint32_t custom_data_id; // 4Bytes: 冷数据索引
    float _m_padding_0;      // 4Bytes 对齐到32
    
    Face() : 
		first_edge(INVALID_EDGE), 
		normal(Vector3Df::ZAxisVector()),
		edge_count(0), 
		material_id(0), 
		flags(0), 
		custom_data_id(UINT32_MAX) {}

    bool IsValid() const noexcept { return first_edge != INVALID_EDGE; }
    bool IsTriangle() const noexcept { return edge_count == 3; }
    bool IsQuad() const noexcept { return edge_count == 4; }
};

struct VertexCustomData {
    Vector2Df uv;               // UV坐标
    Vector3Df color;            // 顶点颜色
    float weight;           // 权重（骨骼动画等）
    uint32_t bone_ids[4];   // 骨骼ID
    float bone_weights[4];  // 骨骼权重
};

struct EdgeCustomData {
    float crease;           // 细分曲面折痕
    float bevel_weight;     // 倒角权重
};

struct FaceCustomData {
    Vector2Df uv_coords[4];     // 每个顶点的UV（最多4个）
    Vector3Df vertex_colors[4]; // 顶点颜色
};
class Mesh {
public:
    // ===== 构造函数 =====
    Mesh() = default;
    
    // ===== 顶点操作 =====
    VertexIndex AddVertex(const Vector3Df& position) {
        VertexIndex idx = vertices.allocate();
        vertices[idx].position = position;
        vertices[idx].normal = Vector3Df::ZAxisVector();
        vertices[idx].outgoing_edge = INVALID_EDGE;
        return idx;
    }
    
    void RemoveVertex(VertexIndex idx) {
        assert(idx < vertices.size());
        vertices[idx].outgoing_edge = INVALID_EDGE;
        vertices.deallocate(idx);
    }
    
    Vertex& GetVertexAt(VertexIndex idx) { return vertices[idx]; }
    const Vertex& GetVertexAt(VertexIndex idx) const { return vertices[idx]; }
    
    // ===== 半边操作 =====
    EdgeIndex AddEdge(VertexIndex v_target, FaceIndex f) {
        EdgeIndex idx = halfedges.allocate();
        halfedges[idx].vertex = v_target;
        halfedges[idx].face = f;
        return idx;
    }
    
    HalfEdge& Edge(EdgeIndex idx) { return halfedges[idx]; }
    const HalfEdge& Edge(EdgeIndex idx) const { return halfedges[idx]; }
    
    // ===== 面操作 =====
    FaceIndex AddFace(const std::vector<VertexIndex>& vertex_indices) {
        if (vertex_indices.size() < 3) return INVALID_FACE;
        
        FaceIndex face_idx = faces.allocate();
        Face& face = faces[face_idx];
        face.edge_count = static_cast<uint16_t>(vertex_indices.size());
        
        // 创建半边循环
        std::vector<EdgeIndex> edges;
        edges.reserve(vertex_indices.size());
        for (size_t i = 0; i < vertex_indices.size(); ++i) {
            EdgeIndex e = AddEdge(vertex_indices[i], face_idx);
            edges.push_back(e);
        }
        
        // 链接半边
        for (size_t i = 0; i < edges.size(); ++i) {
            halfedges[edges[i]].next = edges[(i + 1) % edges.size()];
            halfedges[edges[i]].prev = edges[(i + edges.size() - 1) % edges.size()];
        }
        
        face.first_edge = edges[0];
        
        // 更新顶点的出边
        for (size_t i = 0; i < vertex_indices.size(); ++i) {
            const VertexIndex v = vertex_indices[i];
            if (vertices[v].outgoing_edge == INVALID_EDGE) {
                vertices[v].outgoing_edge = edges[i];
            }
        }
        
        return face_idx;
    }
    
    Face& GetFaceAt(FaceIndex idx) { return faces[idx]; }
    const Face& GetFaceAt(FaceIndex idx) const { return faces[idx]; }
    
    void SetVertexUV(VertexIndex idx, const Vector2Df& uv) {
        uint32_t data_id = vertices[idx].custom_data_idx;
        if (data_id == UINT32_MAX) {
            data_id = static_cast<uint32_t>(vertex_custom_data.size());
            vertex_custom_data.emplace_back();
            vertices[idx].custom_data_idx = data_id;
        }
        vertex_custom_data[data_id].uv = uv;
    }
    
    // ===== 缓存友好的遍历 =====
    template<typename Func>
    void ForEachVertex(Func&& func) {
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (vertices[i].IsValid()) {
                func(static_cast<VertexIndex>(i), vertices[i]);
            }
        }
    }
    
    template<typename Func>
    void ForEachFace(Func&& func) {
        for (size_t i = 0; i < faces.size(); ++i) {
            if (faces[i].IsValid()) {
                func(static_cast<FaceIndex>(i), faces[i]);
            }
        }
    }
    
    // ===== 拓扑查询（带预取优化）=====
    void GetVertexNeighbors(VertexIndex v_idx, std::vector<VertexIndex>& neighbors) {
        neighbors.clear();
        
        EdgeIndex start_edge = vertices[v_idx].outgoing_edge;
        if (start_edge == INVALID_EDGE) return;
        
        EdgeIndex current_edge = start_edge;
        do {
            // 预取下一条边的数据
            EdgeIndex next_edge = halfedges[current_edge].next;
            __builtin_prefetch(&halfedges[next_edge], 0, 3);
            
            EdgeIndex twin_edge = halfedges[current_edge].twin;
            if (twin_edge != INVALID_EDGE) {
                neighbors.push_back(halfedges[twin_edge].vertex);
                current_edge = halfedges[twin_edge].next;
            } else {
                break; // 边界边
            }
        } while (current_edge != start_edge);
    }
    
    // ===== 统计信息 =====
    size_t VertexCount() const { return vertices.size(); }
    size_t EdgeCount() const { return halfedges.size(); }
    size_t FaceCount() const { return faces.size(); }
    
private:
    // ===== 热数据（连续存储）=====
    MemoryPool<Vertex> vertices;
    MemoryPool<HalfEdge> halfedges;
    MemoryPool<Face> faces;
    
    // ===== 冷数据（独立存储）=====
    std::vector<VertexCustomData> vertex_custom_data;
    std::vector<EdgeCustomData> edge_custom_data;
    std::vector<FaceCustomData> face_custom_data;
};

}