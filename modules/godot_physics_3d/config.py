def can_build(env, platform):
    return not env["disable_physics_3d"]


def configure(env):
    pass


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_godot_physics_3d_enabled=yes to enable it.
    return False
