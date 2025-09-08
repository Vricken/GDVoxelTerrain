#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace godot {

struct HermiteSample {
    glm::vec3 p;
    glm::vec3 n; // unit
    glm::vec4 c;
    float w = 1.0f;
};

bool edge_zero_cross(float va, float vb, float& t_out);
HermiteSample interpolate_sample(const glm::vec3& pa, const glm::vec3& pb,
                                 const glm::vec3& na, const glm::vec3& nb,
                                 const glm::vec4& ca, const glm::vec4& cb,
                                 float t);

struct QEF {
    glm::mat3 AtA{0.0f};
    glm::vec3 Atb{0.0f};
    int num = 0;

    void add(const glm::vec3& p, const glm::vec3& n, float w = 1.0f);
    bool solve(glm::vec3& out, const glm::vec3& target, float lambda = 1e-6f) const;
};

glm::vec3 clamp_to_cell(const std::vector<glm::vec3>& pts, const glm::vec3& x);

int face_bit_from_vals(float v0, float v1);
int pack_face_dirs(const std::vector<int>& N, const class StitchedMeshChunk* mc);
}