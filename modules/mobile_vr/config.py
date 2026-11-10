def can_build(env, platform):
    return not env["disable_xr"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "MobileVRInterface",
    ]


def get_doc_path():
    return "doc_classes"


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_mobile_vr_enabled=yes to enable it.
    return False
