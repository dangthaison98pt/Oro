/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "register_types.h"

#include "mono_behaviour.h"
#include "sorting_group.h"
#include "unity_singletons.h"
#include "unity_transform.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"

static Debug *debug_singleton = nullptr;
static Mathf *mathf_singleton = nullptr;
static Random *random_singleton = nullptr;
static SceneManager *scene_manager_singleton = nullptr;
static Application *application_singleton = nullptr;

void initialize_unity_api_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(MonoBehaviour);
	GDREGISTER_CLASS(SortingGroup);
	GDREGISTER_ABSTRACT_CLASS(Transform);
	GDREGISTER_ABSTRACT_CLASS(Debug);
	GDREGISTER_ABSTRACT_CLASS(Mathf);
	GDREGISTER_ABSTRACT_CLASS(Random);
	GDREGISTER_ABSTRACT_CLASS(SceneManager);
	GDREGISTER_ABSTRACT_CLASS(Application);
	GDREGISTER_ABSTRACT_CLASS(KeyCode);

	debug_singleton = memnew(Debug);
	mathf_singleton = memnew(Mathf);
	random_singleton = memnew(Random);
	scene_manager_singleton = memnew(SceneManager);
	application_singleton = memnew(Application);

	Engine::get_singleton()->add_singleton(Engine::Singleton("Debug", debug_singleton));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Mathf", mathf_singleton));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Random", random_singleton));
	Engine::get_singleton()->add_singleton(Engine::Singleton("SceneManager", scene_manager_singleton));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Application", application_singleton));
}

void uninitialize_unity_api_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->remove_singleton("Debug");
	Engine::get_singleton()->remove_singleton("Mathf");
	Engine::get_singleton()->remove_singleton("Random");
	Engine::get_singleton()->remove_singleton("SceneManager");
	Engine::get_singleton()->remove_singleton("Application");

	memdelete(debug_singleton);
	memdelete(mathf_singleton);
	memdelete(random_singleton);
	memdelete(scene_manager_singleton);
	memdelete(application_singleton);
}
