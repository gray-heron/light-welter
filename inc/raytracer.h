
#pragma once
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

#include "log.h"
#include "mesh.h"
#include "scene.h"

struct KDLeaf
{
    // See Implementation Note 1
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
};

struct TriangleIndices
{
    uint32_t t1_, t2_, t3_;
    uint16_t object_id_;

    TriangleIndices(uint32_t t1, uint32_t t2, uint32_t t3, uint16_t object_id)
        : t1_(t1), t2_(t2), t3_(t3), object_id_(object_id)
    {
    }
};

struct TriangleIntersection
{
    TriangleIntersection(float dist, glm::vec3 global_pos, glm::vec3 barycentric_pos,
                         glm::vec3 normal)
        : dist_(dist), global_pos_(global_pos), barycentric_pos_(barycentric_pos),
          normal_(normal)
    {
    }

    float dist_;
    glm::vec3 global_pos_;
    glm::vec3 barycentric_pos_;
    glm::vec3 normal_;
};

class Raytracer
{
    std::array<std::function<bool(const TriangleIndices &i1, const TriangleIndices &i2)>,
               3>
        indices_comparers_min_, indices_comparers_max_;

    const int max_triangles_in_kdleaf_;
    const int kd_max_depth_;

    bool CompareIndices(int dim, bool min, const TriangleIndices &i1,
                        const TriangleIndices &i2) const;
    bool CompareIndicesToAAPlane(int dim, bool min, const TriangleIndices &i1,
                                 float plane) const;

    // returns the position in the kd_tree_
    void KDTreeConstructStep(unsigned int position,
                             std::vector<TriangleIndices>::iterator table_start,
                             std::vector<TriangleIndices>::iterator table_end,
                             std::vector<TriangleIndices> &carry, int current_depth);

    boost::optional<std::pair<TriangleIntersection, TriangleIndices>>
    TraverseKdTree(glm::vec3 lower_bound, glm::vec3 upper_bound, const glm::vec3 &origin,
                   const glm::vec3 &direction, const KDElement &current_node,
                   int current_depth = 0) const;

    boost::optional<std::pair<TriangleIntersection, TriangleIndices>>
    CheckKdLeaf(const glm::vec3 &lower_bound, const glm::vec3 &upper_bound,
                const glm::vec3 &origin, const glm::vec3 &direction,
                const KDLeaf &leaf) const;

    Log log_{"Raytracer"};

    std::vector<KDElement> kd_tree_;
    std::vector<TriangleIndices> indices_;

    int kd_elements_ = 0;
    int total_depth_ = 0;
    int leafs_ = 0;

  public:
    Raytracer(Scene &&scene);

    boost::optional<Intersection>
    Trace(glm::vec3 source, glm::vec3 target,
          boost::optional<uint32_t> max_depth = boost::none) const;

    const Scene scene_;
};