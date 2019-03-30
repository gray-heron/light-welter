

#include "raytracer.h"
#include "config.h"
#include "exceptions.h"

using namespace std::placeholders;
/*
KDElement::KDElement(int32_t first_index, int32_t indices_no)
{
    leaf_.indices_no_ = indices_no;
    leaf_.neg_first_index_ = -first_index;
}

KDElement::KDElement(int32_t first_child, float division)
{
    node_.first_child_ = first_child;
    node_.division_ = division;
}
*/

// https://gamedev.stackexchange.com/questions/133109/m%C3%B6ller-trumbore-false-positive
// dist, intersection pos, barycentric, normal
boost::optional<std::tuple<float, glm::vec3, glm::vec3, glm::vec3>>
RayIntersectsTriangle(const glm::vec3 orig, const glm::vec3 ray, const glm::vec3 vert0,
                      const glm::vec3 vert1, const glm::vec3 vert2)

{
    glm::vec2 result;

    const glm::vec3 edge1 = vert1 - vert0;
    const glm::vec3 edge2 = vert2 - vert0;
    const glm::vec3 pvec = glm::cross(ray, edge2);
    const float det = glm::dot(edge1, pvec);

    const float epsilon = 32.0f * std::numeric_limits<float>::epsilon();

    if (det > -epsilon && det < epsilon)
        return boost::none;

    const float invDet = 1.0f / det;

    const glm::vec3 tvec = orig - vert0;

    result.x = glm::dot(tvec, pvec) * invDet;
    if (result.x < 0.0f || result.x > 1.0f)
        return boost::none;

    const glm::vec3 qvec = glm::cross(tvec, edge1);

    result.y = glm::dot(ray, qvec) * invDet;
    if (result.y < 0.0f || result.x + result.y > 1.0f)
        return boost::none;

    float t = glm::dot(edge2, qvec) * invDet;

    if (t < epsilon)
        return boost::none;

    // glm::vec3 intersection = vert0 + result.x * edge2 + result.y * edge1;
    glm::vec3 intersection = orig + ray * t;

    auto normal = glm::normalize(glm::cross(edge1, edge2));

    return std::tuple<float, glm::vec3, glm::vec3, glm::vec3>(
        t, intersection, glm::vec3(1.0f - (result.x + result.y), result.x, result.y),
        normal);
}

bool CollidesWithAABB(glm::vec3 lower_bound, glm::vec3 upper_bound, glm::vec3 origin,
                      glm::vec3 direction)
{
}

boost::optional<Intersection> Raytracer::Trace(glm::vec3 source, glm::vec3 target,
                                               const Scene &scene) const
{
    boost::optional<Intersection> intersection_so_far;

    RaytracingContext ctx;
    ctx.lights_ = scene.point_lights_;
    ctx.ambient_ = scene.ambient_light_;

    if (auto intersection = scene.mesh_->Raytrace(source, target, ctx, 1))
    {
        intersection_so_far = intersection;
    }

    return intersection_so_far;
}

Raytracer::Raytracer(Scene &&scene)
    : scene_(std::move(scene)), indices_sorters_({
                                    std::bind(&Raytracer::CompareIndicesX, this, _1, _2),
                                    std::bind(&Raytracer::CompareIndicesY, this, _1, _2),
                                    std::bind(&Raytracer::CompareIndicesZ, this, _1, _2),
                                }),
      max_triangles_in_kdleaf_(Config::inst().GetOption<int>("max_triangles_in_kdleaf"))
{
    std::vector<TriangleIndices> indices_vector;

    uint16_t submesh_id = 0;
    for (auto submesh : scene_.mesh_->m_Entries)
    {
        ASSERT(submesh.first.indices_.size() % 3 == 0);
        log_.Info() << "Loading submesh with " << submesh.first.indices_.size() / 3
                    << " triangles.";

        for (int iid = 0; iid < submesh.first.indices_.size(); iid += 3)
        {
            indices_vector.emplace_back(submesh.first.indices_[iid + 0],
                                        submesh.first.indices_[iid + 1],
                                        submesh.first.indices_[iid + 2], submesh_id);
        }

        submesh_id++;
    }

    log_.Info() << submesh_id << " submeshes loaded, " << indices_vector.size()
                << " triangles in total.";
    log_.Info() << "Constructing KD-tree...";

    kd_tree_.resize(indices_vector.size());
    indices_.reserve(indices_vector.size());

    std::vector<TriangleIndices> empty_carry;
    KDTreeConstructStep(0, indices_vector.begin(), indices_vector.end(), empty_carry, 0);

    log_.Info() << "KD-tree construction done. Total leafs: " << leafs_
                << ", average depth: " << float(total_depth_) / float(leafs_);
}

void Raytracer::KDTreeConstructStep(int position,
                                    std::vector<TriangleIndices>::iterator table_start,
                                    std::vector<TriangleIndices>::iterator table_end,
                                    std::vector<TriangleIndices> &carry,
                                    int current_depth)
{
    if (kd_tree_.size() <= position)
    {
        log_.Warning() << "Resizing the kd-tree's buffer!";
        kd_tree_.resize(position + 1);
    }

    int triagles_no = (table_end - table_start) + carry.size();

    if (triagles_no > max_triangles_in_kdleaf_)
    {
        int split_dimension = current_depth % 3;
        std::sort(table_start, table_end, indices_sorters_[split_dimension]);

        auto pivot = table_start + ((table_end - table_start) / 2);
        std::vector<TriangleIndices> carry_left, carry_right;

        for (const auto &triagle : carry)
        {
            if (indices_sorters_[split_dimension](triagle, *pivot))
                carry_left.emplace_back(triagle);
            else
                carry_right.emplace_back(triagle);
        }

        const auto &mv = scene_.mesh_->m_Entries[pivot->object_id_].first.vertices_;
        float split = std::max(std::max(mv[pivot->t1_].pos_[split_dimension],
                                        mv[pivot->t2_].pos_[split_dimension]),
                               mv[pivot->t3_].pos_[split_dimension]);

        int first_child_id = kd_elements_ + 1;
        kd_tree_[position].node_ = {first_child_id, split};
        kd_elements_ += 2;
        KDTreeConstructStep(first_child_id, table_start, pivot, carry_left,
                            current_depth + 1);
        KDTreeConstructStep(first_child_id + 1, pivot, table_end, carry_right,
                            current_depth + 1);
    }
    else
    {
        int indices_position = indices_.size();

        kd_tree_[position].leaf_ = {-indices_position, triagles_no};

        std::copy(table_start, table_end, back_inserter(indices_));
        std::copy(carry.begin(), carry.end(), back_inserter(indices_));

        leafs_ += 1;
        total_depth_ += current_depth;
    }
}

boost::optional<TriangleIndices>
Raytracer::TraverseKdTree(glm::vec3 lower_bound, glm::vec3 upper_bound,
                          const glm::vec3 &origin, const glm::vec3 &direction,
                          const KDElement &current_node, int current_depth) const
{
    if (!CollidesWithAABB(lower_bound, upper_bound, origin, direction))
        return boost::none;

    if (current_node.leaf_.neg_first_index_ > 0)
    {
        int split_dimension = current_depth % 3;
        auto middle_lower_bound = lower_bound;
        auto middle_upper_bound = upper_bound;

        middle_lower_bound[split_dimension] = current_node.node_.division_;
        middle_upper_bound[split_dimension] = current_node.node_.division_;

        auto &child1 = kd_tree_[current_node.node_.first_child_];
        auto &child2 = kd_tree_[current_node.node_.first_child_ + 1];

        if (direction[split_dimension] > 0.0f)
        {
            if (auto collision = TraverseKdTree(lower_bound, middle_upper_bound, origin,
                                                direction, child1, current_depth + 1))
                return collision;
            else
                return TraverseKdTree(middle_lower_bound, upper_bound, origin, direction,
                                      child2, current_depth + 1);
        }
        else
        {
            if (auto collision = TraverseKdTree(middle_lower_bound, upper_bound, origin,
                                                direction, child2, current_depth + 1))
                return collision;
            else
                return TraverseKdTree(lower_bound, middle_upper_bound, origin, direction,
                                      child1, current_depth + 1);
        }
    }
    else
    {
        // we are leaf
        return CheckKdLeaf(origin, direction, current_node.leaf_);
    }
}

boost::optional<TriangleIndices>
Raytracer::CheckKdLeaf(glm::vec3 origin, glm::vec3 direction, const KDLeaf &leaf) const
{
    int indices_start_index = -leaf.neg_first_index_;

    boost::optional<TriangleIndices> intersection_so_far;
    auto intersection_dist_so_far = std::numeric_limits<float>::infinity();

    glm::vec3 pos, normal;
    glm::vec3 barycentric;
    float intersection_dist;

    for (unsigned int i = indices_start_index; i < leaf.indices_no_; i += 1)
    {
        const auto &mv = scene_.mesh_->m_Entries[indices_[i].object_id_].first.vertices_;

        auto vertex1 = mv[indices_[i].t1_];
        auto vertex2 = mv[indices_[i].t2_];
        auto vertex3 = mv[indices_[i].t3_];

        if (auto intersection = RayIntersectsTriangle(origin, direction, vertex1.pos_,
                                                      vertex2.pos_, vertex3.pos_))
        {
            std::tie(intersection_dist, pos, barycentric, normal) = *intersection;

            if (intersection_dist < intersection_dist_so_far)
            {
                intersection_so_far = indices_[i];
                intersection_dist_so_far = intersection_dist;
            }
        }
    }

    return intersection_so_far;
}

#define CompareIndicesIMPL(NAME, DIM)                                                    \
    bool NAME(const TriangleIndices &i1, const TriangleIndices &i2) const                \
    {                                                                                    \
        const auto &mv1 = scene_.mesh_->m_Entries[i1.object_id_].first.vertices_;        \
        const auto &mv2 = scene_.mesh_->m_Entries[i2.object_id_].first.vertices_;        \
                                                                                         \
        return std::max(std::max(mv1[i1.t1_].pos_[DIM], mv1[i1.t2_].pos_[DIM]),          \
                        mv1[i1.t3_].pos_[DIM]) <=                                        \
               std::max(std::max(mv2[i2.t1_].pos_[DIM], mv2[i2.t2_].pos_[DIM]),          \
                        mv2[i2.t3_].pos_[DIM]);                                          \
    }

CompareIndicesIMPL(Raytracer::CompareIndicesX, 0);
CompareIndicesIMPL(Raytracer::CompareIndicesY, 1);
CompareIndicesIMPL(Raytracer::CompareIndicesZ, 2);