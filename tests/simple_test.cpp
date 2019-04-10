#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test of tests"

#include "mesh.h"

#include <algorithm>
#include <boost/test/unit_test.hpp>

float TriangleArea(const Vertex &v1, const Vertex &v2, const Vertex &v3);

BOOST_AUTO_TEST_CASE(TriangleAreaTest)
{
    Vertex v1, v2, v3;
    v1.pos_ = glm::vec3(10.0f, 10.0f, 20.0f);
    v2.pos_ = glm::vec3(11.0f, 10.0f, 20.0f);
    v3.pos_ = glm::vec3(11.0f, 10.0f, 21.0f);

    BOOST_CHECK_CLOSE(TriangleArea(v1, v2, v3), 0.5f, 0.01f);
};
