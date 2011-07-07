/*
 * BVH.cc
 *
 *  Created on: 23/05/2011
 *      Author: jaubry
 */

#ifndef BVH_HH
#define BVH_HH

#include <vector>
#include <stack>
#include <limits>
#include <algorithm>

#include "BVHNode.hh"
#include "Geometry.hh"

namespace BASim
{

typedef BoundingBox<Scalar> BBoxType;
typedef BVHNode<BBoxType> BVHNodeType;

class BVH
{
public:
    typedef BVHNodeType Node_Type; // for pantaray::kNN<BVH>

    /// empty constructor
    BVH()
    {
    }

    /// returns the size of this object in bytes
    size_t ByteSize() const
    {
        return sizeof(BVHNodeType) * m_nodes.size() + sizeof(BVH);
    }

    /// get node vector
    const std::vector<BVHNodeType>& GetNodeVector() const
    {
        return m_nodes;
    }

    /// get node vector
    std::vector<BVHNodeType>& GetNodeVector()
    {
        return m_nodes;
    }

    /// get nodes pointer
    const BVHNodeType* GetNodes() const
    {
        return &m_nodes[0];
    }

    /// get nodes pointer
    BVHNodeType* GetNodes()
    {
        return &m_nodes[0];
    }

    /// get the i-th node
    const BVHNodeType& GetNode(const uint32_t i) const
    {
        return m_nodes[i];
    }

    /// get the i-th node
    BVHNodeType& GetNode(const uint32_t i)
    {
        return m_nodes[i];
    }

private:
    std::vector<BVHNodeType> m_nodes; ///< bvh nodes
};

void swap(BVH& a, BVH& b);

template<typename BBoxFunctorT>
class BVHBuilder
{
public:
    typedef BBoxType::PointType PointType;

    /// empty constructor
    BVHBuilder() :
        m_max_leaf_size(1u)
    {
    }

    void build(BBoxFunctorT& bboxes, BVH* bvh);

private:
    BBoxType presplit(const BBoxType& node_bbox, const BBoxType& kd_bbox);

    struct StackNode
    {
        StackNode()
        {
        }
        StackNode(const uint32_t node, const uint32_t begin, const uint32_t end, const uint32_t depth, const BBoxType& kd_bbox) :
            m_node_index(node), m_begin(begin), m_end(end), m_depth(depth), m_kd_bbox(kd_bbox)
        {
        }

        uint32_t m_node_index;
        uint32_t m_begin;
        uint32_t m_end;
        uint32_t m_depth;
        BBoxType m_kd_bbox;
    };

    BVH* m_bvh; ///< output bvh
    std::stack<StackNode> m_stack; ///< internal stack
    const uint32_t m_max_leaf_size;///< maximum leaf size
};

bool do_overlap(const BBoxType& bbox1, const BBoxType& bbox2);

bool is_contained(const BBoxType& bbox1, const BBoxType& bbox2, const Scalar tol = 0.0f);

BBoxType intersection(const BBoxType& bbox1, const BBoxType& bbox2);

typedef BBoxType BBoxType;

bool is_left(const BBoxType& bbox, const uint32_t axis, const Scalar pivot);

void insert(BBoxType& bbox, const BBoxType& bbox2);

BBoxType merge(const BBoxType& bbox1, const BBoxType& bbox2);

template<typename BBoxFunctorT>
uint32_t partition(BBoxFunctorT& bboxes, const uint32_t begin, const uint32_t end, const uint32_t axis, const Scalar pivot);

template<typename BBoxFunctorT>
BBoxType compute_bbox(BBoxFunctorT& bboxes, const uint32_t begin, const uint32_t end);

}

#endif
