#include "dc_math.h"
#include "voxel_terrain/meshing/mesh_chunk.h"
#include <cmath>
#include <limits>
#include <algorithm>

using namespace godot;

bool godot::edge_zero_cross(float va, float vb, float& t_out) {
    if ((va >= 0.0f && vb >= 0.0f) || (va <= 0.0f && vb <= 0.0f)) return false;
    const float denom = std::abs(va) + std::abs(vb);
    if (denom <= 1e-12f) return false;
    t_out = std::abs(va) / denom;
    return true;
}

HermiteSample godot::interpolate_sample(const glm::vec3& pa, const glm::vec3& pb,
                                 const glm::vec3& na, const glm::vec3& nb,
                                 const glm::vec4& ca, const glm::vec4& cb,
                                 float t) {
    HermiteSample hs;
    hs.p = glm::mix(pa, pb, t);
    hs.n = glm::normalize(glm::mix(na, nb, t));
    hs.c = glm::mix(ca, cb, t);
    hs.w = 1.0f;
    return hs;
}

void QEF::add(const glm::vec3& p, const glm::vec3& n, float w) {
    const glm::vec3 nu = glm::normalize(n);
    const float b = glm::dot(nu, p);
    AtA += (w) * glm::outerProduct(nu, nu);
    Atb += (w) * (nu * b);
    ++num;
}

bool QEF::solve(glm::vec3& out, const glm::vec3& target, float lambda) const {
    glm::mat3 M = AtA + glm::mat3(lambda);
    glm::vec3 rhs = Atb + lambda * target;
    float det = glm::determinant(M);
    if (std::abs(det) < 1e-12f) return false;
    out = glm::inverse(M) * rhs;
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

glm::vec3 godot::clamp_to_cell(const std::vector<glm::vec3>& pts, const glm::vec3& x) {
    glm::vec3 mn(std::numeric_limits<float>::infinity());
    glm::vec3 mx(-std::numeric_limits<float>::infinity());
    for (const auto& p : pts) {
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
    }
    const glm::vec3 eps(1e-6f);
    return glm::clamp(x, mn - eps, mx + eps);
}

int godot::face_bit_from_vals(float v0, float v1) {
    const bool neg0 = v0 < 0.0f, neg1 = v1 < 0.0f;
    if (neg0 == neg1) return 0;
    return neg0 ? -1 : +1;
}

// NOTE: Update corner index pairs to match your corner indexing
int godot::pack_face_dirs(const std::vector<int>& N, const MeshChunk* mc) {
    const float vx0 = mc->nodes[N[0]]->get_value();
    const float vx1 = mc->nodes[N[1]]->get_value();
    const float vy0 = mc->nodes[N[0]]->get_value();
    const float vy1 = mc->nodes[N[2]]->get_value();
    const float vz0 = mc->nodes[N[0]]->get_value();
    const float vz1 = mc->nodes[N[4]]->get_value();

    int fx = face_bit_from_vals(vx0, vx1);
    int fy = face_bit_from_vals(vy0, vy1);
    int fz = face_bit_from_vals(vz0, vz1);

    auto pack2 = [](int s) { return (s + 1) & 0x3; };
    return (pack2(fx) << 0) | (pack2(fy) << 2) | (pack2(fz) << 4);
}
