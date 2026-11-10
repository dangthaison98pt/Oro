def can_build(env, platform):
    env.module_add_dependencies("fbx", ["gltf"])
    return not env["disable_3d"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "EditorSceneFormatImporterFBX2GLTF",
        "EditorSceneFormatImporterUFBX",
        "FBXDocument",
        "FBXState",
    ]


def get_doc_path():
    return "doc_classes"


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_fbx_enabled=yes to enable it.
    return False
