

#include "raycaster.h"
#include "config.h"
#include "exceptions.h"

using namespace std::placeholders;

extern std::string S(glm::vec4 in);
extern std::string S(glm::vec3 in);

const float EPSILON = 0.001f;
glm::vec3 EPSILON3 = glm::vec3(EPSILON, EPSILON, EPSILON);

// https://gamedev.stackexchange.com/questions/133109/m%C3%B6ller-trumbore-false-positive
boost::optional<TriangleIntersection>
RayIntersectsTriangle(const glm::vec3 orig, const glm::vec3 ray, const glm::vec3 vert0,
                      const glm::vec3 vert1, const glm::vec3 vert2)

{
    glm::vec2 result;

    const glm::vec3 edge1 = vert1 - vert0;
    const glm::vec3 edge2 = vert2 - vert0;
    const glm::vec3 pvec = glm::cross(ray, edge2);
    const float det = glm::dot(edge1, pvec);

    if (det > -EPSILON && det < EPSILON)
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

    if (t < EPSILON)
        return boost::none;

    glm::vec3 intersection = orig + ray * t;

    auto normal = glm::normalize(glm::cross(edge1, edge2));

    return TriangleIntersection(
        t, intersection, glm::vec3(1.0f - (result.x + result.y), result.x, result.y),
        normal);
}

bool Beetween(float v, float low, float high) { return low <= v && v <= high; }

bool PointInAABB(glm::vec3 point, glm::vec3 lower_bound, glm::vec3 upper_bound)
{
    return Beetween(point.x, lower_bound.x, upper_bound.x) &&
           Beetween(point.y, lower_bound.y, upper_bound.y) &&
           Beetween(point.z, lower_bound.z, upper_bound.z);
}

bool CollidesWithAABB(glm::vec3 lower_bound, glm::vec3 upper_bound, glm::vec3 origin,
                      glm::vec3 direction)
{
    if (PointInAABB(origin, lower_bound, upper_bound))
        return true;

    float tmin = std::numeric_limits<float>::min(),
          tmax = std::numeric_limits<float>::max();

    glm::vec3 inv_direction = 1.0f / direction;
    glm::vec3 t1 = (lower_bound - origin) * inv_direction;
    glm::vec3 t2 = (upper_bound - origin) * inv_direction;

    for (int i = 0; i < 3; i++)
    {
        tmin = std::max(tmin, std::min(t1[i], t2[i]));
        tmax = std::min(tmax, std::max(t1[i], t2[i]));
    }

    return tmax >= tmin;
}

boost::optional<std::pair<TriangleIntersection, TriangleIndices>>
RayCaster::Trace(glm::vec3 source, glm::vec3 dir) const
{
    return TraverseKdTree(mesh_->GetLowerBound(), mesh_->GetUpperBound(), source, dir,
                          kd_tree_[0]);
}

RayCaster::RayCaster(std::shared_ptr<Mesh> mesh)
    : indices_comparers_min_(
          {std::bind(&RayCaster::CompareIndices, this, 0, true, _1, _2),
           std::bind(&RayCaster::CompareIndices, this, 1, true, _1, _2),
           std::bind(&RayCaster::CompareIndices, this, 2, true, _1, _2)}),
      indices_comparers_max_(
          {std::bind(&RayCaster::CompareIndices, this, 0, false, _1, _2),
           std::bind(&RayCaster::CompareIndices, this, 1, false, _1, _2),
           std::bind(&RayCaster::CompareIndices, this, 2, false, _1, _2)}),
      max_triangles_in_kdleaf_(
          Config::inst().GetOption<int>("kdtree_max_triangles_in_leaf")),
      kd_max_depth_(Config::inst().GetOption<int>("kdtree_max_depth")),
      sah_resolution_(Config::inst().GetOption<int>("sah_resolution")), mesh_(mesh)

{
    std::vector<TriangleIndices> indices_vector;

    uint16_t submesh_id = 0;
    for (auto submesh : mesh_->m_Entries)
    {
        STRONG_ASSERT(submesh.first.indices_.size() % 3 == 0);
        log_.Info() << "Loading submesh with " << submesh.first.indices_.size() / 3
                    << " triangles.";

        for (unsigned int iid = 0; iid < submesh.first.indices_.size(); iid += 3)
        {
            indices_vector.emplace_back(submesh.first.indices_[iid + 0],
                                        submesh.first.indices_[iid + 1],
                                        submesh.first.indices_[iid + 2], submesh_id);
        }

        submesh_id++;
    }

    log_.Info() << mesh_->m_Entries.size() << " submeshes loaded, "
                << indices_vector.size() << " triangles in total.";
    log_.Info() << "Constructing KD-tree...";

    kd_tree_.resize(indices_vector.size());
    indices_.reserve(indices_vector.size());
    sah_segments_.resize(sah_resolution_);

    std::vector<TriangleIndices> empty_carry;

    KDTreeConstructStep(0, indices_vector.begin(), indices_vector.end(), empty_carry, 0);

    log_.Info() << "KD-tree construction done. Total leafs: " << leafs_
                << ", average depth: " << float(total_depth_) / float(leafs_);
}

float TriangleArea(const Vertex &v1, const Vertex &v2, const Vertex &v3)
{
    glm::vec3 ab = v2.pos_ - v1.pos_;
    glm::vec3 ac = v3.pos_ - v1.pos_;

    return glm::length(glm::cross(ab, ac)) / 2.0f;
}

std::vector<TriangleIndices>::iterator
RayCaster::SurfaceAreaHeuristic(std::vector<TriangleIndices>::iterator left,
                                std::vector<TriangleIndices>::iterator right,
                                int split_dimension)
{
    std::fill(sah_segments_.begin(), sah_segments_.end(), 0);

    float area_left = 0.0f, area_right = 0.0f;
    const int triangles_no = right - left;
    const int triangles_per_segment = (triangles_no / sah_resolution_) + 1;
    int segments_used = 0;

    const float ll = TriangleMax(split_dimension, *left);
    const float lr = TriangleMax(split_dimension, *(right - 1));

    if (lr - ll < 0.001f)
    {
        log_.Warning() << "SAH called on super small triangle range. Returning middle!";
        return left + triangles_no / 2;
    }

    std::vector<TriangleIndices>::iterator iter = left;

    for (float &segment : sah_segments_)
    {
        for (int i = 0; i < triangles_per_segment; i++)
        {
            if (iter == right)
                break;

            const std::vector<Vertex> &mv =
                mesh_->m_Entries[iter->object_id_].first.vertices_;

            float current_area =
                TriangleArea(mv[iter->t1_], mv[iter->t2_], mv[iter->t3_]);

            segment += current_area;
            area_right += current_area;

            ++iter;
        }

        segments_used += 1;

        if (iter == right)
            break;
    }

    int best_split = -1;
    float best_split_value = std::numeric_limits<float>::max();

    for (int i = 0; i < segments_used - 1; i++)
    {
        if (i * triangles_per_segment >= triangles_no)
            break;

        area_left += sah_segments_[i];
        area_right -= sah_segments_[i];

        float size_left =
            (TriangleMax(split_dimension, *(left + i * triangles_per_segment)) - ll) /
            (lr - ll);

        STRONG_ASSERT(size_left <= 1.0f);
        STRONG_ASSERT(size_left >= 0.0f);

        float current_sah_value = size_left * area_left + (1.0f - size_left) * area_right;

        STRONG_ASSERT(current_sah_value > 0.0f);

        // log_.Info() << "Split at "
        //            << TriangleMax(split_dimension, *(left + i * triangles_per_segment))
        //            << " has value " << current_sah_value;

        if (current_sah_value < best_split_value)
        {
            best_split_value = current_sah_value;
            best_split = i;
        }
    }

    STRONG_ASSERT(best_split != -1);

    // log_.Info() << "Choosing split at " << best_split;

    return left + best_split * triangles_per_segment;
}

void RayCaster::KDTreeConstructStep(unsigned int position,
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

    if (triagles_no > max_triangles_in_kdleaf_ && current_depth < kd_max_depth_ &&
        (table_end - table_start) > carry.size())
    {
        int split_dimension = current_depth % 3;
        std::sort(table_start, table_end, indices_comparers_max_[split_dimension]);

        std::vector<TriangleIndices>::iterator pivot;
        float split;

        if (sah_resolution_ > 0)
            pivot = SurfaceAreaHeuristic(table_start, table_end, split_dimension);
        else
            // sah disabled, just use mean
            pivot = table_start + ((table_end - table_start) / 2);

        const std::vector<Vertex> &mv =
            mesh_->m_Entries[pivot->object_id_].first.vertices_;

        split = std::max(std::max(mv[pivot->t1_].pos_[split_dimension],
                                  mv[pivot->t2_].pos_[split_dimension]),
                         mv[pivot->t3_].pos_[split_dimension]);

        std::vector<TriangleIndices> carry_left, carry_right;

        // see Implementation Note 2
        for (const auto &triagle : carry)
        {
            if (CompareIndicesToAAPlane(split_dimension, true, triagle, split))
                carry_left.emplace_back(triagle);
            if (!CompareIndicesToAAPlane(split_dimension, false, triagle, split))
                carry_right.emplace_back(triagle);
        }

        // see Implementation Note 3
        for (auto i = pivot; i < table_end; ++i)
        {
            if (CompareIndicesToAAPlane(split_dimension, true, *i, split))
                carry_left.emplace_back(*i);
        }

        int first_child_id = kd_elements_ + 1;
        kd_tree_[position].node_ = {first_child_id, split};
        kd_elements_ += 2;
        KDTreeConstructStep(first_child_id + 1, pivot, table_end, carry_right,
                            current_depth + 1);
        KDTreeConstructStep(first_child_id, table_start, pivot, carry_left,
                            current_depth + 1);
    }
    else
    {
        int indices_position = indices_.size();

        // See Implementation Note 1
        kd_tree_[position].leaf_ = {-indices_position, triagles_no};

        std::copy(table_start, table_end, back_inserter(indices_));
        std::copy(carry.begin(), carry.end(), back_inserter(indices_));

        leafs_ += 1;
        total_depth_ += current_depth;
    }
}

boost::optional<std::pair<TriangleIntersection, TriangleIndices>>
RayCaster::TraverseKdTree(glm::vec3 lower_bound, glm::vec3 upper_bound,
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
        auto intersection =
            CheckKdLeaf(lower_bound, upper_bound, origin, direction, current_node.leaf_);

        return intersection;
    }
}

boost::optional<std::pair<TriangleIntersection, TriangleIndices>>
RayCaster::CheckKdLeaf(const glm::vec3 &lower_bound, const glm::vec3 &upper_bound,
                       const glm::vec3 &origin, const glm::vec3 &direction,
                       const KDLeaf &leaf) const
{
    int indices_start_index = -leaf.neg_first_index_;

    boost::optional<std::pair<TriangleIntersection, TriangleIndices>> intersection_so_far;
    auto intersection_dist_so_far = std::numeric_limits<float>::infinity();

    for (int i = indices_start_index; i < indices_start_index + leaf.indices_no_; i += 1)
    {
        const auto &mv = mesh_->m_Entries[indices_[i].object_id_].first.vertices_;
        const auto &vertex1 = mv[indices_[i].t1_];
        const auto &vertex2 = mv[indices_[i].t2_];
        const auto &vertex3 = mv[indices_[i].t3_];

        if (auto intersection = RayIntersectsTriangle(origin, direction, vertex1.pos_,
                                                      vertex2.pos_, vertex3.pos_))
        {
            if (intersection->dist_ < intersection_dist_so_far &&
                PointInAABB(intersection->global_pos_, lower_bound - EPSILON3,
                            upper_bound + EPSILON3))
            {
                intersection_so_far = std::pair<TriangleIntersection, TriangleIndices>(
                    *intersection, indices_[i]);
                intersection_dist_so_far = intersection->dist_;
            }
        }
    }

    return intersection_so_far;
}

float RayCaster::TriangleMax(int dim, const TriangleIndices &i1)
{
    const auto &mv = mesh_->m_Entries[i1.object_id_].first.vertices_;

    return std::max(std::max(mv[i1.t1_].pos_[dim], mv[i1.t2_].pos_[dim]),
                    mv[i1.t3_].pos_[dim]);
}

bool RayCaster::CompareIndices(int dim, bool min, const TriangleIndices &i1,
                               const TriangleIndices &i2) const
{
    const auto &mv1 = mesh_->m_Entries[i1.object_id_].first.vertices_;
    const auto &mv2 = mesh_->m_Entries[i2.object_id_].first.vertices_;

    if (min)
        return std::min(std::min(mv1[i1.t1_].pos_[dim], mv1[i1.t2_].pos_[dim]),
                        mv1[i1.t3_].pos_[dim]) <
               std::min(std::min(mv2[i2.t1_].pos_[dim], mv2[i2.t2_].pos_[dim]),
                        mv2[i2.t3_].pos_[dim]);
    else
        return std::max(std::max(mv1[i1.t1_].pos_[dim], mv1[i1.t2_].pos_[dim]),
                        mv1[i1.t3_].pos_[dim]) <
               std::max(std::max(mv2[i2.t1_].pos_[dim], mv2[i2.t2_].pos_[dim]),
                        mv2[i2.t3_].pos_[dim]);
}

bool RayCaster::CompareIndicesToAAPlane(int dim, bool min, const TriangleIndices &i1,
                                        float plane) const
{
    const auto &mv1 = mesh_->m_Entries[i1.object_id_].first.vertices_;

    if (min)
        return std::min(std::min(mv1[i1.t1_].pos_[dim], mv1[i1.t2_].pos_[dim]),
                        mv1[i1.t3_].pos_[dim]) < plane;
    else
        return std::max(std::max(mv1[i1.t1_].pos_[dim], mv1[i1.t2_].pos_[dim]),
                        mv1[i1.t3_].pos_[dim]) < plane;
}