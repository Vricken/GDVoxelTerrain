#include "register_types.h"
#include "box_sdf.h"
#include "planar_world.h"
#include "plane_sdf.h"
#include "planet_sdf.h"
#include "operation_sdf.h"
#include "sphere_sdf.h"
#include "sdf_modification.h"
#include "spherical_world.h"
#include "terrain_detail.h"
#include "terrain_feature.h"
#include "terrain_populator.h"
#include "terrain_sdf.h"
#include "mesh_sdf.h"
#include "transformed_sdf.h"
#include "voxel_chunk.h"
#include "voxel_terrain.h"
#include "world.h"

using namespace godot;

void initialize_jar_voxel_terrain_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        // SDFs
        GDREGISTER_ABSTRACT_CLASS(JarSignedDistanceField);
        GDREGISTER_CLASS(JarOperationSdf);
        GDREGISTER_CLASS(JarBoxSdf);
        GDREGISTER_CLASS(JarSphereSdf);
        GDREGISTER_CLASS(JarPlaneSdf);
        GDREGISTER_CLASS(JarTerrainSdf);
        GDREGISTER_CLASS(JarPlanetSdf);
        GDREGISTER_CLASS(JarTransformedSdf);
        GDREGISTER_CLASS(JarMeshSdf);

        GDREGISTER_CLASS(JarSdfModification);

        // Make sure enum constants are available during script parsing.  Binding
        // them only in _bind_methods can sometimes be too late, causing GDScript
        // to report missing members (see terrain_editor.gd errors).  To be safe
        // we register them immediately when the module is initialized.
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_UNION", SDF_OPERATION_UNION);
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_SUBTRACTION", SDF_OPERATION_SUBTRACTION);
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_INTERSECTION", SDF_OPERATION_INTERSECTION);
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_SMOOTH_UNION", SDF_OPERATION_SMOOTH_UNION);
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_SMOOTH_SUBTRACTION", SDF_OPERATION_SMOOTH_SUBTRACTION);
        ClassDB::add_integer_constant("JarSdfModification", "SDF_OPERATION_SMOOTH_INTERSECTION", SDF_OPERATION_SMOOTH_INTERSECTION);

        // WORLD
        GDREGISTER_ABSTRACT_CLASS(JarWorld);
        GDREGISTER_CLASS(JarPlanarWorld);
        GDREGISTER_CLASS(JarSphericalWorld);

        // POPULATION
        GDREGISTER_ABSTRACT_CLASS(JarTerrainPopulator);
        GDREGISTER_CLASS(JarTerrainDetail);
        GDREGISTER_CLASS(JarTerrainFeature);

        // TERRAIN
        GDREGISTER_CLASS(JarVoxelTerrain);
        GDREGISTER_CLASS(JarVoxelChunk);
    }
}

void uninitialize_jar_voxel_terrain_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
    }
}

extern "C"
{
    // Initialization.
    GDExtensionBool GDE_EXPORT jar_voxel_terrain_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                              const GDExtensionClassLibraryPtr p_library,
                                                              GDExtensionInitialization *r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_jar_voxel_terrain_module);
        init_obj.register_terminator(uninitialize_jar_voxel_terrain_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}