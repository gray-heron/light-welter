
#pragma once
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

#include "log.h"
#include "mesh.h"
#include "scene.h"

/*
 To maximally reduce the size of the kd-tree node, there is no explicit
 leaf\node flag. Since both have normally unsigned integer on the first
 4 bytes, leafs have their one negated.
*/

struct KDLeaf
{
    int32_t neg_first_index_;
    int32_t indices_no_;
};

struct KDNode
{
    int32_t first_child_;
    float division_;
};

union KDElement {
    KDLeaf leaf_;
    KDNode node_;
    /*
        // Construct leaf, watch out for the int\float abiguity
        KDElement(int32_t first_index, int32_t indices_no);

        // Construct node
        KDElement(int32_t first_child, float division);
        */
};

struct TriangleIndices
{
    uint32_t t1_, t2_, t3_;
    uint16_t object_id_;

    TriangleIndices(uint32_t t1, uint32_t t2, uint32_t t3, uint16_t object_id)
        : t1_(t1), t2_(t2), t3_(t3)
    {
    }
};

class Raytracer
{
    Scene scene_;
    std::array<std::function<bool(const TriangleIndices &i1, const TriangleIndices &i2)>,
               3>
        indices_sorters_;
    const int max_triangles_in_kdleaf_;

    bool CompareIndicesX(const TriangleIndices &i1, const TriangleIndices &i2) const;
    bool CompareIndicesY(const TriangleIndices &i1, const TriangleIndices &i2) const;
    bool CompareIndicesZ(const TriangleIndices &i1, const TriangleIndices &i2) const;

    // returns the position in the kd_tree_
    void KDTreeConstructStep(int position,
                             std::vector<TriangleIndices>::iterator table_start,
                             std::vector<TriangleIndices>::iterator table_end,
                             std::vector<TriangleIndices> &carry, int current_depth);

    boost::optional<TriangleIndices>
    TraverseKdTree(glm::vec3 lower_bound, glm::vec3 upper_bound, const glm::vec3 &origin,
                   const glm::vec3 &direction, const KDElement &current_node,
                   int current_depth = 0) const;

    boost::optional<TriangleIndices> CheckKdLeaf(glm::vec3 origin, glm::vec3 direction,
                                                 const KDLeaf &leaf) const;

    Log log_{"Raytracer"};

    std::vector<KDElement> kd_tree_;
    int kd_elements_ = 0;
    int total_depth_ = 0;
    int leafs_ = 0;

  public:
    Raytracer(Scene &&scene);

    boost::optional<Intersection> Trace(glm::vec3 source, glm::vec3 target,
                                        const Scene &scene) const;

    std::vector<TriangleIndices> indices_;
};