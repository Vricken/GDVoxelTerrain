#ifndef JAR_AABB_H
#define JAR_AABB_H

#include <glm/glm.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/string.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace godot {

struct Bounds
{
  public:
    glm::vec3 min_corner;
    glm::vec3 max_corner;

    Bounds() : min_corner(std::numeric_limits<float>::max()), max_corner(std::numeric_limits<float>::lowest())
    {
    }

    Bounds(const glm::vec3 &min, const glm::vec3 &max)
    {
        this->min_corner = glm::min(min, max);
        this->max_corner = glm::max(min, max);
    }

    Bounds(const godot::AABB &aabb)
    {
        godot::Vector3 godot_min = aabb.position;
        godot::Vector3 godot_max = aabb.position + aabb.size;
        min_corner = {godot_min.x, godot_min.y, godot_min.z};
        max_corner = {godot_max.x, godot_max.y, godot_max.z};
        // Ensure validity
        min_corner = glm::min(min_corner, max_corner);
        max_corner = glm::max(min_corner, max_corner);
    }

    inline bool is_valid() const
    {
        return (min_corner.x <= max_corner.x && min_corner.y <= max_corner.y && min_corner.z <= max_corner.z);
    }

    inline glm::vec3 get_center() const
    {
        return (min_corner + max_corner) * 0.5f;
    }

    inline glm::vec3 get_size() const
    {
        return max_corner - min_corner;
    }

    inline bool contains_point(const glm::vec3 &point) const
    {
        return (point.x >= min_corner.x && point.x <= max_corner.x && point.y >= min_corner.y && point.y <= max_corner.y && point.z >= min_corner.z &&
                point.z <= max_corner.z);
    }

    inline Bounds recentered(const glm::vec3 &new_center) const
    {
        glm::vec3 half_size = get_size() * 0.5f;
        return Bounds(new_center - half_size, new_center + half_size);
    }

    inline Bounds intersected(const Bounds &other) const
    {
        glm::vec3 new_min = glm::max(min_corner, other.min_corner);
        glm::vec3 new_max = glm::min(max_corner, other.max_corner);
        if (new_min.x > new_max.x || new_min.y > new_max.y || new_min.z > new_max.z)
        {
            return Bounds(); // Empty bounds
        }
        return Bounds(new_min, new_max);
    }

    inline Bounds joined(const Bounds &other) const
    {
        glm::vec3 new_min = glm::min(min_corner, other.min_corner);
        glm::vec3 new_max = glm::max(max_corner, other.max_corner);
        return Bounds(new_min, new_max);
    }

    // assumption: other or this do not enclose each other.
    inline Bounds subtracted(const Bounds &other) const
    {
        if (!intersects(other))
        {
            return *this; // No intersection, return the original bounds
        }

        glm::vec3 new_min = min_corner;
        glm::vec3 new_max = max_corner;

        // Remove the overlapping volume
        if (other.min_corner.x <= max_corner.x && other.max_corner.x >= min_corner.x)
        {
            if (other.min_corner.x > min_corner.x)
            {
                new_max.x = glm::min(max_corner.x, other.min_corner.x);
            }
            else if (other.max_corner.x < max_corner.x)
            {
                new_min.x = glm::max(min_corner.x, other.max_corner.x);
            }
        }

        if (other.min_corner.y <= max_corner.y && other.max_corner.y >= min_corner.y)
        {
            if (other.min_corner.y > min_corner.y)
            {
                new_max.y = glm::min(max_corner.y, other.min_corner.y);
            }
            else if (other.max_corner.y < max_corner.y)
            {
                new_min.y = glm::max(min_corner.y, other.max_corner.y);
            }
        }

        if (other.min_corner.z <= max_corner.z && other.max_corner.z >= min_corner.z)
        {
            if (other.min_corner.z > min_corner.z)
            {
                new_max.z = glm::min(max_corner.z, other.min_corner.z);
            }
            else if (other.max_corner.z < max_corner.z)
            {
                new_min.z = glm::max(min_corner.z, other.max_corner.z);
            }
        }

        return Bounds(new_min, new_max);
    }

    inline Bounds shaved_by_closest_plane(const Bounds &other) const {
        if (other.contains_point(this->get_center())) {
            return *this;
        }
    
        const glm::vec3 this_center = get_center();
        const glm::vec3 other_center = other.get_center();
        const glm::vec3 d = this_center - other_center;
        const glm::vec3 ad = glm::abs(d);
        const float min_ad = glm::min(glm::min(ad.x, ad.y), ad.z);
    
        // Mask for axes with smallest distance component
        const glm::vec3 mask = glm::step(ad, glm::vec3(min_ad)) * 
                              glm::step(glm::vec3(min_ad), ad);
        
        // Direction mask (1 for positive d, 0 for negative)
        const glm::vec3 use_max = glm::step(glm::vec3(0.0f), d);
    
        // Apply adjustment only to closest axis
        const glm::vec3 new_min = min_corner * (1.0f - mask) + 
                                 (other.min_corner * (1.0f - use_max) + min_corner * use_max) * mask;
        const glm::vec3 new_max = max_corner * (1.0f - mask) + 
                                 (other.max_corner * use_max + max_corner * (1.0f - use_max)) * mask;
    
        return Bounds(glm::min(new_min, new_max), glm::max(new_min, new_max));
    }

    inline Bounds expanded(const glm::vec3 &amount) const
    {
        return Bounds(min_corner - amount, max_corner + amount);
    }

    inline Bounds expanded(float amount) const
    {
        return expanded(glm::vec3(amount));
    }

    inline Bounds min(const Bounds &other) const
    {
        return Bounds(glm::min(min_corner, other.min_corner), glm::min(max_corner, other.max_corner));
    }

    inline Bounds max(const Bounds &other) const
    {
        return Bounds(glm::max(min_corner, other.min_corner), glm::max(max_corner, other.max_corner));
    }

    inline bool intersects(const Bounds &other) const
    {
        return (min_corner.x <= other.max_corner.x && max_corner.x >= other.min_corner.x && min_corner.y <= other.max_corner.y && max_corner.y >= other.min_corner.y &&
                min_corner.z <= other.max_corner.z && max_corner.z >= other.min_corner.z);
    }

    inline bool encloses(const Bounds &other) const
    {
        return (min_corner.x <= other.min_corner.x && max_corner.x >= other.max_corner.x && min_corner.y <= other.min_corner.y && max_corner.y >= other.max_corner.y &&
                min_corner.z <= other.min_corner.z && max_corner.z >= other.max_corner.z);
    }

    inline Bounds operator*(float scalar) const
    {
        return Bounds(min_corner * scalar, max_corner * scalar);
    }

    inline Bounds operator*(const glm::vec3 &vec) const
    {
        return Bounds(min_corner * vec, max_corner * vec);
    }

    inline Bounds operator+(const glm::vec3 &offset) const
    {
        return Bounds(min_corner + offset, max_corner + offset);
    }

    godot::String to_string() const
    {
        return godot::String(("{" + glm::to_string(min_corner) + ", " + glm::to_string(max_corner) + "}").c_str());
    }

};
}

#endif // JAR_AABB_H