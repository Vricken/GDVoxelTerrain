#ifndef SDF_OPERATIONS_H
#define SDF_OPERATIONS_H

#include <algorithm>
#include <glm/glm.hpp>

enum SDFOperation
{
    SDF_OPERATION_UNION,
    SDF_OPERATION_SUBTRACTION,
    SDF_OPERATION_INTERSECTION,
    SDF_OPERATION_SMOOTH_UNION,
    SDF_OPERATION_SMOOTH_SUBTRACTION,
    SDF_OPERATION_SMOOTH_INTERSECTION,
};

namespace SDF
{

// --- Scalar only ---
static inline float apply_operation(SDFOperation op, float a, float b, float k,
                                    float &outFactor) // 0 = b, 1 = a
{
    switch (op)
    {
    case SDF_OPERATION_UNION:
        if (a < b)
        {
            outFactor = 1.0f;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            return b;
        }

    case SDF_OPERATION_SUBTRACTION:
        if (a > -b)
        {
            outFactor = 1.0f;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            return -b;
        }

    case SDF_OPERATION_INTERSECTION:
        if (a > b)
        {
            outFactor = 1.0f;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            return b;
        }

    case SDF_OPERATION_SMOOTH_UNION: {
        float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
        outFactor = h;
        return glm::mix(b, a, h) - k * h * (1.0f - h);
    }

    case SDF_OPERATION_SMOOTH_SUBTRACTION: {
        float h = std::clamp(0.5f - 0.5f * (b + a) / k, 0.0f, 1.0f);
        outFactor = h;
        return glm::mix(a, -b, h) + k * h * (1.0f - h);
    }

    case SDF_OPERATION_SMOOTH_INTERSECTION: {
        float h = std::clamp(0.5f - 0.5f * (b - a) / k, 0.0f, 1.0f);
        outFactor = h;
        return glm::mix(b, a, h) + k * h * (1.0f - h);
    }

    default:
        outFactor = 1.0f;
        return a;
    }
}

// --- With normals ---
static inline float apply_operation(SDFOperation op, float a, const glm::vec3 &na, float b, const glm::vec3 &nb,
                                    float k, glm::vec3 &outNormal,
                                    float &outFactor) // 0 = b, 1 = a
{
    switch (op)
    {
    case SDF_OPERATION_UNION:
        if (a < b)
        {
            outFactor = 1.0f;
            outNormal = na;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            outNormal = nb;
            return b;
        }

    case SDF_OPERATION_SUBTRACTION:
        if (a > -b)
        {
            outFactor = 1.0f;
            outNormal = na;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            outNormal = -nb;
            return -b;
        }

    case SDF_OPERATION_INTERSECTION:
        if (a > b)
        {
            outFactor = 1.0f;
            outNormal = na;
            return a;
        }
        else
        {
            outFactor = 0.0f;
            outNormal = nb;
            return b;
        }

    case SDF_OPERATION_SMOOTH_UNION: {
        float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
        outFactor = h;
        outNormal = glm::normalize(glm::mix(nb, na, h));
        return glm::mix(b, a, h) - k * h * (1.0f - h);
    }

    case SDF_OPERATION_SMOOTH_SUBTRACTION: {
        float h = std::clamp(0.5f - 0.5f * (b + a) / k, 0.0f, 1.0f);
        outFactor = h;
        outNormal = glm::normalize(glm::mix(na, -nb, h));
        return glm::mix(a, -b, h) + k * h * (1.0f - h);
    }

    case SDF_OPERATION_SMOOTH_INTERSECTION: {
        float h = std::clamp(0.5f - 0.5f * (b - a) / k, 0.0f, 1.0f);
        outFactor = h;
        outNormal = glm::normalize(glm::mix(nb, na, h));
        return glm::mix(b, a, h) + k * h * (1.0f - h);
    }

    default:
        outFactor = 1.0f;
        outNormal = na;
        return a;
    }
}

} // namespace SDF

VARIANT_ENUM_CAST(SDFOperation);

#endif // SDF_OPERATIONS_H
