def can_build(env, platform):
    env.module_add_dependencies("gltf", ["csg", "gridmap"], True)
    return not env["disable_3d"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "EditorSceneFormatImporterBlend",
        "EditorSceneFormatImporterGLTF",
        "GLTFAccessor",
        "GLTFAnimation",
        "GLTFBufferView",
        "GLTFCamera",
        "GLTFDocument",
        "GLTFDocumentExtension",
        "GLTFDocumentExtensionConvertImporterMesh",
        "GLTFLight",
        "GLTFMesh",
        "GLTFNode",
        "GLTFObjectModelProperty",
        "GLTFPhysicsBody",
        "GLTFPhysicsShape",
        "GLTFSkeleton",
        "GLTFSkin",
        "GLTFSpecGloss",
        "GLTFState",
        "GLTFTexture",
        "GLTFTextureSampler",
    ]


def get_doc_path():
    return "doc_classes"


def is_enabled():
    # Oro is a 2D-only engine: this 3D module is disabled by default.
    # Use module_gltf_enabled=yes to enable it.
    return False
