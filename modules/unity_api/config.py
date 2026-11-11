def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "MonoBehaviour",
        "SortingGroup",
        "Transform",
        "Debug",
        "Mathf",
        "Random",
        "SceneManager",
        "Application",
        "KeyCode",
    ]


def get_doc_path():
    return "doc_classes"
