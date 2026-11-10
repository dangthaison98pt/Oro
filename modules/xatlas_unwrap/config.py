def can_build(env, platform):
    return env.editor_build


def configure(env):
    pass


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_xatlas_unwrap_enabled=yes to enable it.
    return False
