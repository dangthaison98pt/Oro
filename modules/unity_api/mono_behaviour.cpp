/**************************************************************************/
/*  mono_behaviour.cpp                                                    */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "mono_behaviour.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/rigid_body_2d.h"
#endif // PHYSICS_2D_DISABLED

#define INVOKE_META SNAME("_unity_invoke")

bool MonoBehaviour::_runs_callbacks() const {
	// In the editor, only @tool scripts run (like everywhere else in the engine).
	if (!Engine::get_singleton()->is_editor_hint()) {
		return true;
	}
	const ScriptInstance *instance = get_script_instance();
	return instance && instance->get_script().is_valid() && instance->get_script()->is_tool();
}

Node *MonoBehaviour::unity_get_game_object() {
	// The game object is the node this component is attached to.
	Node *parent = get_parent();
	return parent ? parent : this;
}

Node *MonoBehaviour::GetComponent(const Variant &p_type) {
	return unity_get_game_object()->Node::GetComponent(p_type);
}

TypedArray<Node> MonoBehaviour::GetComponents(const Variant &p_type) {
	return unity_get_game_object()->Node::GetComponents(p_type);
}

Node *MonoBehaviour::GetComponentInChildren(const Variant &p_type) {
	return unity_get_game_object()->Node::GetComponentInChildren(p_type);
}

Node *MonoBehaviour::GetComponentInParent(const Variant &p_type) {
	return unity_get_game_object()->Node::GetComponentInParent(p_type);
}

bool MonoBehaviour::CompareTag(const String &p_tag) {
	return unity_get_game_object()->Node::CompareTag(p_tag);
}

Ref<Transform> MonoBehaviour::get_transform() {
	if (transform.is_null()) {
		transform.instantiate();
	}
	transform->set_target(unity_get_game_object());
	return transform;
}

void MonoBehaviour::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	if (is_inside_tree() && _runs_callbacks()) {
		if (enabled) {
			GDVIRTUAL_CALL(OnEnable);
			if (is_ready() && !started) {
				started = true;
				GDVIRTUAL_CALL(Start);
			}
		} else {
			GDVIRTUAL_CALL(OnDisable);
		}
	}
	_update_processing();
}

bool MonoBehaviour::is_enabled() const {
	return enabled;
}

void MonoBehaviour::_update_processing() {
	const bool runs = enabled && is_inside_tree() && _runs_callbacks();
	set_process(runs && (GDVIRTUAL_IS_OVERRIDDEN(Update) || GDVIRTUAL_IS_OVERRIDDEN(LateUpdate)));
	set_physics_process(runs && GDVIRTUAL_IS_OVERRIDDEN(FixedUpdate));
}

void MonoBehaviour::_connect_physics_signals() {
#ifndef PHYSICS_2D_DISABLED
	Node *game_object = unity_get_game_object();
	if (game_object == this) {
		return;
	}

	Area2D *area = Object::cast_to<Area2D>(game_object);
	if (area) {
		if (GDVIRTUAL_IS_OVERRIDDEN(OnTriggerEnter2D)) {
			area->connect(SNAME("body_entered"), callable_mp(this, &MonoBehaviour::_on_trigger_enter), CONNECT_REFERENCE_COUNTED);
			area->connect(SNAME("area_entered"), callable_mp(this, &MonoBehaviour::_on_trigger_enter), CONNECT_REFERENCE_COUNTED);
		}
		if (GDVIRTUAL_IS_OVERRIDDEN(OnTriggerExit2D)) {
			area->connect(SNAME("body_exited"), callable_mp(this, &MonoBehaviour::_on_trigger_exit), CONNECT_REFERENCE_COUNTED);
			area->connect(SNAME("area_exited"), callable_mp(this, &MonoBehaviour::_on_trigger_exit), CONNECT_REFERENCE_COUNTED);
		}
	}

	RigidBody2D *body = Object::cast_to<RigidBody2D>(game_object);
	if (body && (GDVIRTUAL_IS_OVERRIDDEN(OnCollisionEnter2D) || GDVIRTUAL_IS_OVERRIDDEN(OnCollisionExit2D))) {
		// Like Unity, collision callbacks work without extra setup.
		body->set_contact_monitor(true);
		if (body->get_max_contacts_reported() < 4) {
			body->set_max_contacts_reported(4);
		}
		if (GDVIRTUAL_IS_OVERRIDDEN(OnCollisionEnter2D)) {
			body->connect(SNAME("body_entered"), callable_mp(this, &MonoBehaviour::_on_collision_enter), CONNECT_REFERENCE_COUNTED);
		}
		if (GDVIRTUAL_IS_OVERRIDDEN(OnCollisionExit2D)) {
			body->connect(SNAME("body_exited"), callable_mp(this, &MonoBehaviour::_on_collision_exit), CONNECT_REFERENCE_COUNTED);
		}
	}
#endif // PHYSICS_2D_DISABLED
}

void MonoBehaviour::_on_trigger_enter(Node *p_other) {
	if (enabled) {
		GDVIRTUAL_CALL(OnTriggerEnter2D, p_other);
	}
}

void MonoBehaviour::_on_trigger_exit(Node *p_other) {
	if (enabled) {
		GDVIRTUAL_CALL(OnTriggerExit2D, p_other);
	}
}

void MonoBehaviour::_on_collision_enter(Node *p_other) {
	if (enabled) {
		GDVIRTUAL_CALL(OnCollisionEnter2D, p_other);
	}
}

void MonoBehaviour::_on_collision_exit(Node *p_other) {
	if (enabled) {
		GDVIRTUAL_CALL(OnCollisionExit2D, p_other);
	}
}

void MonoBehaviour::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (!_runs_callbacks()) {
				return;
			}
			if (!awoken) {
				awoken = true;
				GDVIRTUAL_CALL(Awake);
			}
			if (enabled) {
				GDVIRTUAL_CALL(OnEnable);
			}
		} break;

		case NOTIFICATION_READY: {
			_update_processing();
			if (!_runs_callbacks()) {
				return;
			}
			_connect_physics_signals();
			if (enabled && !started) {
				started = true;
				GDVIRTUAL_CALL(Start);
			}
		} break;

		case NOTIFICATION_PROCESS: {
			if (enabled) {
				GDVIRTUAL_CALL(Update);
				GDVIRTUAL_CALL(LateUpdate);
			}
		} break;

		case NOTIFICATION_PHYSICS_PROCESS: {
			if (enabled) {
				GDVIRTUAL_CALL(FixedUpdate);
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (enabled && _runs_callbacks()) {
				GDVIRTUAL_CALL(OnDisable);
			}
		} break;

		case NOTIFICATION_PREDELETE: {
			if (awoken && _runs_callbacks()) {
				GDVIRTUAL_CALL(OnDestroy);
			}
		} break;
	}
}

Node *MonoBehaviour::Instantiate(const Variant &p_original, const Variant &p_position, Node *p_parent) {
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), nullptr, "Instantiate() can only be used while the component is in the scene tree.");

	Node *instance = nullptr;
	const Ref<PackedScene> prefab = p_original;
	if (prefab.is_valid()) {
		instance = prefab->instantiate();
	} else if (Node *original = Object::cast_to<Node>(p_original.get_validated_object())) {
		instance = original->duplicate();
	}
	ERR_FAIL_NULL_V_MSG(instance, nullptr, "Instantiate() expects a prefab (PackedScene, e.g. from preload(\"res://enemy.tscn\")) or a Node.");

	Node *parent = p_parent;
	if (!parent) {
		parent = get_tree()->get_current_scene();
	}
	if (!parent) {
		parent = get_tree()->get_root();
	}

	if (!parent->is_ready()) {
		// The parent is still setting up its children (e.g. Instantiate() called from Awake/Start),
		// so the object is added at the end of the frame, like the engine requires.
		if (p_position.get_type() != Variant::NIL) {
			Transform::for_node(instance)->set_local_position(p_position);
		}
		parent->call_deferred(SNAME("add_child"), instance);
		return instance;
	}

	parent->add_child(instance);
	if (p_position.get_type() != Variant::NIL) {
		Transform::for_node(instance)->set_position(p_position);
	}
	return instance;
}

void MonoBehaviour::Destroy(Object *p_object, double p_delay) {
	Node *node = Object::cast_to<Node>(p_object);
	ERR_FAIL_NULL_MSG(node, "Destroy() expects a game object or a component (a Node).");
	if (p_delay <= 0.0) {
		node->queue_free();
		return;
	}
	Timer *timer = memnew(Timer);
	timer->set_one_shot(true);
	timer->set_wait_time(p_delay);
	timer->set_autostart(true);
	timer->connect(SNAME("timeout"), callable_mp(node, &Node::queue_free));
	node->add_child(timer, false, INTERNAL_MODE_BACK);
}

void MonoBehaviour::Invoke(const StringName &p_method, double p_delay) {
	InvokeRepeating(p_method, p_delay, 0.0);
}

void MonoBehaviour::InvokeRepeating(const StringName &p_method, double p_delay, double p_repeat_rate) {
	Timer *timer = memnew(Timer);
	timer->set_one_shot(true);
	timer->set_wait_time(MAX(p_delay, 0.0001));
	timer->set_autostart(true);
	timer->set_meta(INVOKE_META, p_method);
	timer->connect(SNAME("timeout"), callable_mp(this, &MonoBehaviour::_on_invoke_timeout).bind(timer, p_method, p_repeat_rate));
	add_child(timer, false, INTERNAL_MODE_BACK);
}

void MonoBehaviour::_on_invoke_timeout(Object *p_timer, const StringName &p_method, double p_repeat_rate) {
	Timer *timer = Object::cast_to<Timer>(p_timer);
	ERR_FAIL_NULL(timer);
	if (enabled) {
		call(p_method);
	}
	if (p_repeat_rate > 0.0) {
		timer->start(p_repeat_rate);
	} else {
		timer->queue_free();
	}
}

void MonoBehaviour::CancelInvoke(const StringName &p_method) {
	for (int i = get_child_count(true) - 1; i >= 0; i--) {
		Timer *timer = Object::cast_to<Timer>(get_child(i, true));
		if (timer && timer->has_meta(INVOKE_META) && (p_method == StringName() || StringName(timer->get_meta(INVOKE_META)) == p_method)) {
			timer->stop();
			timer->queue_free();
		}
	}
}

bool MonoBehaviour::IsInvoking(const StringName &p_method) const {
	for (int i = 0; i < get_child_count(true); i++) {
		const Timer *timer = Object::cast_to<Timer>(get_child(i, true));
		if (timer && timer->has_meta(INVOKE_META) && !timer->is_queued_for_deletion() && (p_method == StringName() || StringName(timer->get_meta(INVOKE_META)) == p_method)) {
			return true;
		}
	}
	return false;
}

void MonoBehaviour::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_transform"), &MonoBehaviour::get_transform);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &MonoBehaviour::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &MonoBehaviour::is_enabled);

	ClassDB::bind_method(D_METHOD("Instantiate", "original", "position", "parent"), &MonoBehaviour::Instantiate, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("Destroy", "object", "delay"), &MonoBehaviour::Destroy, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("Invoke", "method", "delay"), &MonoBehaviour::Invoke);
	ClassDB::bind_method(D_METHOD("InvokeRepeating", "method", "delay", "repeat_rate"), &MonoBehaviour::InvokeRepeating);
	ClassDB::bind_method(D_METHOD("CancelInvoke", "method"), &MonoBehaviour::CancelInvoke, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("IsInvoking", "method"), &MonoBehaviour::IsInvoking, DEFVAL(StringName()));

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_transform");

	GDVIRTUAL_BIND(Awake);
	GDVIRTUAL_BIND(Start);
	GDVIRTUAL_BIND(Update);
	GDVIRTUAL_BIND(FixedUpdate);
	GDVIRTUAL_BIND(LateUpdate);
	GDVIRTUAL_BIND(OnEnable);
	GDVIRTUAL_BIND(OnDisable);
	GDVIRTUAL_BIND(OnDestroy);
	GDVIRTUAL_BIND(OnTriggerEnter2D, "other");
	GDVIRTUAL_BIND(OnTriggerExit2D, "other");
	GDVIRTUAL_BIND(OnCollisionEnter2D, "other");
	GDVIRTUAL_BIND(OnCollisionExit2D, "other");
}
