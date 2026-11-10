def can_build(env, platform):
    return not env["disable_physics_3d"]


def configure(env):
    pass


def is_enabled():
    # Orostride is a 2D-only engine: this 3D module is disabled by default.
    # Use module_vhacd_enabled=yes to enable it.
    return False
