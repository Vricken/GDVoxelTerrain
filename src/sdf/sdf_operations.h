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

static inline float apply_operation(SDFOperation op, float a, float b, float k = 1.0f)
{
    switch (op)
    {
    case SDF_OPERATION_UNION:
        return std::min(a, b);
    case SDF_OPERATION_SUBTRACTION:
        return std::max(a, -b);
    case SDF_OPERATION_INTERSECTION:
        return std::max(a, b);
    case SDF_OPERATION_SMOOTH_UNION:
    {
        float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
        return glm::mix(b, a, h) - k * h * (1.0f - h);
    }
    case SDF_OPERATION_SMOOTH_SUBTRACTION:
    {
        float h = std::clamp(0.5f - 0.5f * (b + a) / k, 0.0f, 1.0f);
        return glm::mix(a, -b, h) + k * h * (1.0f - h);
    }
    case SDF_OPERATION_SMOOTH_INTERSECTION:
    {
        float h = std::clamp(0.5f - 0.5f * (b - a) / k, 0.0f, 1.0f);
        return glm::mix(b, a, h) + k * h * (1.0f - h);
    }
    default:
        return a;
    }
}

static inline void apply_operation(
    SDFOperation op,
    float a, const glm::vec3& na,
    float b, const glm::vec3& nb,
    float k,
    float& outValue, glm::vec3& outNormal)
{
    switch (op)
    {
    case SDF_OPERATION_UNION:
        if (a < b) { outValue = a; outNormal = na; }
        else       { outValue = b; outNormal = nb; }
        break;

    case SDF_OPERATION_SUBTRACTION:
        if (a > -b) { outValue = a; outNormal = na; }
        else        { outValue = -b; outNormal = -nb; }
        break;

    case SDF_OPERATION_INTERSECTION:
        if (a > b) { outValue = a; outNormal = na; }
        else       { outValue = b; outNormal = nb; }
        break;

    case SDF_OPERATION_SMOOTH_UNION:
    {
        float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
        outValue = glm::mix(b, a, h) - k * h * (1.0f - h);
        outNormal = glm::normalize(glm::mix(nb, na, h));
    } break;

    case SDF_OPERATION_SMOOTH_SUBTRACTION:
    {
        float h = std::clamp(0.5f - 0.5f * (b + a) / k, 0.0f, 1.0f);
        outValue = glm::mix(a, -b, h) + k * h * (1.0f - h);
        outNormal = glm::normalize(glm::mix(na, -nb, h));
    } break;

    case SDF_OPERATION_SMOOTH_INTERSECTION:
    {
        float h = std::clamp(0.5f - 0.5f * (b - a) / k, 0.0f, 1.0f);
        outValue = glm::mix(b, a, h) + k * h * (1.0f - h);
        outNormal = glm::normalize(glm::mix(nb, na, h));
    } break;

    default:
        outValue = a;
        outNormal = na;
        break;
    }
}


} // namespace SDF

VARIANT_ENUM_CAST(SDFOperation);

#endif // SDF_OPERATIONS_H