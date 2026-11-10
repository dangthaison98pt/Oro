def can_build(env, platform):
    # Having this on release by default, it's small and a lot of users like to do procedural stuff
    return not env["disable_3d"]


def configure(env):
    pass


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_meshoptimizer_enabled=yes to enable it.
    return False
