#define DUAL_CONTOURING_UTILITY_H

#ifndef DUAL_CONTOURING_UTILITY_H
#define DUAL_CONTOURING_UTILITY_H

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct QEF {
    // Accumulated normal equations components
    glm::mat3 AtA{0.0f};
    glm::vec3 Atb{0.0f};
    float btb = 0.0f; // optional, not needed for solve but useful for error

    int num = 0;

    void add(const glm::vec3& p, const glm::vec3& n, float weight = 1.0f) {
        // Rows of A are normals; b_i = n·p. Weighted accumulation.
        glm::vec3 nw = n * weight;
        float bi = glm::dot(nw, p);

        // AtA += n^T n (outer product)
        AtA += glm::outerProduct(nw, n);
        Atb += nw * bi;
        btb += bi * bi;
        ++num;
    }

    // Solve (AtA + lambda*I) x = Atb
    bool solve(glm::vec3& out, float lambda = 1e-6f) const {
        glm::mat3 M = AtA + g#define DUAL_CONTOURING_UTILITY_Hlm::mat3(lambda); // Tikhonov regularization
        float det = glm::determinant(M);
        if (std::abs(det) < 1e-12f) return false;

        glm::mat3 invM = glm::inverse(M);
        out = invM * Atb;
        return true;
    }
};

struct HermiteSample {
    glm::vec3 p;
    glm::vec3 n;  // should be unit length
    glm::vec4 c;  // color at intersection (optional)
    float w = 1.0f; // weight
};

// Linear interpolation parameter along edge AB where values cross zero
inline bool edge_zero_cross(
    float va, float vb, float& t_out)
{
    if ((va >= 0.0f && vb >= 0.0f) || (va <= 0.0f && vb <= 0.0f)) return false;
    float denom = std::abs(va) + std::abs(vb);
    if (denom <= 1e-12f) return false; // degenerate
    t_out = std::abs(va) / denom; // same as va / (va - vb) but robust for signs
    return true;
}

inline HermiteSample interpolate_sample(
    const glm::vec3& pa, const glm::vec3& pb,
    const glm::vec3& na, const glm::vec3& nb,
    const glm::vec4& ca, const glm::vec4& cb,
    float t)
{
    HermiteSample hs;
    hs.p = glm::mix(pa, pb, t);
    hs.n = glm::normalize(glm::mix(na, nb, t)); // or recompute n from SDF if available
    hs.c = glm::mix(ca, cb, t);
    hs.w = 1.0f;
    return hs;
}

struct DCSolveResult {
    bool valid = false;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec4 color{0.0f};
    int crossings = 0;
};

DCSolveResult solve_dual_contouring_vertex(
    const std::vector<int>& neighbours,
    const StitchedMeshChunk& meshChunk,
    const glm::vec3& cellMin,  // optional: cell bounds for clamping
    const glm::vec3& cellMax,  // optional: cell bounds for clamping
    float lambda = 1e-6f)
{
    DCSolveResult out;
    std::vector<HermiteSample> samples;
    samples.reserve(12);

    // 1) Collect Hermite samples from edge zero-crossings
    for (auto& e : StitchedMeshChunk::Edges) {
        int ai = neighbours[e.x];
        int bi = neighbours[e.y];

        auto na = meshChunk.nodes[ai];
        auto nb = meshChunk.nodes[bi];

        float va = na->get_value();
        float vb = nb->get_value();

        float t;
        if (!edge_zero_cross(va, vb, t)) continue;

        HermiteSample hs = interpolate_sample(
            na->_center, nb->_center,
            na->get_normal(), nb->get_normal(),
            na->get_color(), nb->get_color(),
            t);

        samples.push_back(hs);
    }

    if (samples.empty()) return out; // no crossings
    out.crossings = static_cast<int>(samples.size());

    // 2) Build QEF
    QEF qef;
    glm::vec3 centroid{0.0f};
    glm::vec4 colorAcc{0.0f};
    float colorW = 0.0f;

    for (const auto& s : samples) {
        qef.add(s.p, s.n, s.w);
        centroid += s.p;
        colorAcc += s.c;
        colorW += 1.0f;
    }
    centroid *= (1.0f / float(samples.size()));
    glm::vec4 avgColor = (colorW > 0.0f) ? (colorAcc * (1.0f / colorW)) : glm::vec4(0.0f);

    // 3) Solve QEF (regularized)
    glm::vec3 x;
    bool ok = qef.solve(x, lambda);

    // 4) Fallbacks for degeneracy
    if (!ok || !glm::all(glm::isfinite(x))) {
        x = centroid; // stable fallback
    }

    // 5) Clamp to cell bounds to avoid vertices drifting outside
    auto clamp3 = [](const glm::vec3& p, const glm::vec3& mn, const glm::vec3& mx) {
        return glm::clamp(p, mn, mx);
    };
    x = clamp3(x, cellMin, cellMax);

    // 6) Output normal: use the best-conditioned aggregate
    // Here we average and renormalize. You can also sample SDF gradient at x.
    glm::vec3 nsum{0.0f};
    for (const auto& s : samples) {
        // Weight normals more if the plane x is close: wi = 1 / (epsilon + |n·(x - p)|)
        float d = std::abs(glm::dot(s.n, x - s.p));
        float w = 1.0f / (1e-4f + d);
        nsum += s.n * w;
    }
    glm::vec3 n = glm::normalize(nsum);

    out.valid = true;
    out.position = x;
    out.normal = n;
    out.color = avgColor;
    return out;
}
#endif // DUAL_CONTOURING_UTILITY_H