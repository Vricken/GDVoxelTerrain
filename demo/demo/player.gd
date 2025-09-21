class_name Player
extends CharacterBody3D

@onready var yaw_pivot: Node3D = $YawPivot/Camera3D
@onready var camera: Camera3D = $YawPivot/Camera3D
@export var terrain: JarVoxelTerrain

# --- Config ---
@export var fly_speed: float = 8.0
@export var look_sensitivity: float = 1.0
const MIN_FLY_SPEED := 0.5
const MAX_FLY_SPEED := 4096
const WALK_SPEED := 5.0
const JUMP_VELOCITY := 4.5

# --- State ---
enum State { WALK, FLY }
var state: State = State.WALK
var is_rotating := false

func _ready() -> void:
	set_mouse(true)

func set_mouse(value: bool) -> void:
	is_rotating = value
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if value else Input.MOUSE_MODE_VISIBLE

func toggle_state() -> void:
	state = State.FLY if state == State.WALK else State.WALK

func _input(event) -> void:
	if event.is_action_pressed("scroll_up"):
		fly_speed = clamp(fly_speed * 2.0, MIN_FLY_SPEED, MAX_FLY_SPEED)
	if event.is_action_pressed("scroll_down"):
		fly_speed = clamp(fly_speed * 0.5, MIN_FLY_SPEED, MAX_FLY_SPEED)

	if event.is_action_pressed("ui_cancel"):
		set_mouse(!is_rotating)
	if event.is_action_pressed("ui_focus_next"):
		toggle_state()

	if event is InputEventMouseMotion and is_rotating:
		camera.rotation.x = clamp(
			camera.rotation.x - event.relative.y * 0.0025 * look_sensitivity,
			-PI/2.2, PI/2.2
		)
		yaw_pivot.rotation.y -= event.relative.x * 0.0025 * look_sensitivity

	if event.is_action_pressed("terrain_update_lod"):
		terrain.force_update_lod()

func align_with_y(xform: Transform3D, new_y: Vector3) -> Transform3D:
	xform.basis.y = new_y
	xform.basis.x = -xform.basis.z.cross(new_y)
	xform.basis = xform.basis.orthonormalized()
	return xform

func _physics_process(delta: float) -> void:
	var gravity := get_gravity()
	var planet_up := -gravity.normalized()
	global_transform = align_with_y(global_transform, planet_up)

	match state:
		State.WALK:
			_process_walk(delta, gravity)
		State.FLY:
			_process_fly(delta)

	move_and_slide()

# --- WALKING ---
func _process_walk(delta: float, gravity: Vector3) -> void:
	var input_vec = Input.get_vector("move_left", "move_right", "move_forward", "move_backward")
	var forward = yaw_pivot.global_basis.z
	var right = yaw_pivot.global_basis.x
	var move_dir = (forward * input_vec.y + right * input_vec.x).normalized()

	if move_dir != Vector3.ZERO:
		velocity.x = move_dir.x * WALK_SPEED
		velocity.z = move_dir.z * WALK_SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, WALK_SPEED)
		velocity.z = move_toward(velocity.z, 0, WALK_SPEED)

	# gravity + jump
	if not is_on_floor():
		velocity += gravity * delta
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

# --- FLYING ---
func _process_fly(_delta: float) -> void:
	var input_vec = Input.get_vector("move_left", "move_right", "move_forward", "move_backward")
	var forward = camera.global_basis.z
	var right = camera.global_basis.x
	var up = Vector3.ZERO
	if Input.is_action_pressed("move_up"):
		up = Vector3.UP
	if Input.is_action_pressed("move_down"):
		up = Vector3.DOWN

	var move_dir = (forward * input_vec.y + right * input_vec.x + up).normalized()
	velocity = move_dir * fly_speed
