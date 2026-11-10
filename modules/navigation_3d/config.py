def can_build(env, platform):
    if env["disable_navigation_3d"]:
        return False

    env.module_add_dependencies("navigation", ["csg", "gridmap"], True)
    return True


def configure(env):
    pass


def is_enabled():
    # Orostride is a 2D-only engine: this 3D module is disabled by default.
    # Use module_navigation_3d_enabled=yes to enable it.
    return False
