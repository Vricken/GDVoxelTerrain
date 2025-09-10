extends Node3D

@export var terrain: JarVoxelTerrain
@export var sdf_modification: JarSdfModification

var edit_timer = 0.0
func _physics_process(delta: float) -> void:
	if Input.is_action_pressed("left_click"):
		_edit(true);
	if Input.is_action_pressed("right_click"):
		_edit(false)
		
	edit_timer -= delta

func _edit(union : bool):
	if(edit_timer > 0):
		return;
	edit_timer = 0.05
	var origin = global_position;
	var direction = -global_transform.basis.z;
	var space_state = get_world_3d().direct_space_state
	var query = PhysicsRayQueryParameters3D.create(origin, origin + direction * 1000)
	var result = space_state.intersect_ray(query)
	if result:
		sdf_modification.operation = JarSdfModification.SDF_OPERATION_UNION if union else JarSdfModification.SDF_OPERATION_SUBTRACTION
		sdf_modification.center = result.position
		terrain.modify_using_sdf(sdf_modification)
		#terrain.spawn_debug_spheres_in_bounds(result.position, 16)
