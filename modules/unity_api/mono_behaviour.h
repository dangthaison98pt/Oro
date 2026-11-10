/**************************************************************************/
/*  mono_behaviour.h                                                      */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "unity_transform.h"

#include "core/object/gdvirtual.gen.h"
#include "scene/main/node.h"

class Timer;

// Unity-style script component. Add it (or a script extending it) as a child of a game object:
// `gameObject` is the parent node and `transform` controls the parent's position, rotation and scale.
// Script callbacks use Unity's names (Start, Update, OnTriggerEnter2D, ...).
class MonoBehaviour : public Node {
	GDCLASS(MonoBehaviour, Node);

	bool enabled = true;
	bool awoken = false;
	bool started = false;
	Ref<Transform> transform;

	bool _runs_callbacks() const;
	void _update_processing();
	void _connect_physics_signals();

	void _on_trigger_enter(Node *p_other);
	void _on_trigger_exit(Node *p_other);
	void _on_collision_enter(Node *p_other);
	void _on_collision_exit(Node *p_other);
	void _on_invoke_timeout(Object *p_timer, const StringName &p_method, double p_repeat_rate);

protected:
	void _notification(int p_what);
	static void _bind_methods();

	// Unity callbacks (PascalCase on purpose).
	GDVIRTUAL0(Awake)
	GDVIRTUAL0(Start)
	GDVIRTUAL0(Update)
	GDVIRTUAL0(FixedUpdate)
	GDVIRTUAL0(LateUpdate)
	GDVIRTUAL0(OnEnable)
	GDVIRTUAL0(OnDisable)
	GDVIRTUAL0(OnDestroy)
	GDVIRTUAL1(OnTriggerEnter2D, Node *)
	GDVIRTUAL1(OnTriggerExit2D, Node *)
	GDVIRTUAL1(OnCollisionEnter2D, Node *)
	GDVIRTUAL1(OnCollisionExit2D, Node *)

public:
	virtual Node *unity_get_game_object() override;
	virtual Node *GetComponent(const Variant &p_type) override;
	virtual TypedArray<Node> GetComponents(const Variant &p_type) override;
	virtual Node *GetComponentInChildren(const Variant &p_type) override;
	virtual Node *GetComponentInParent(const Variant &p_type) override;
	virtual bool CompareTag(const String &p_tag) override;

	Ref<Transform> get_transform();
	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	Node *Instantiate(const Variant &p_original, const Variant &p_position = Variant(), Node *p_parent = nullptr);
	void Destroy(Object *p_object, double p_delay = 0.0);
	void Invoke(const StringName &p_method, double p_delay);
	void InvokeRepeating(const StringName &p_method, double p_delay, double p_repeat_rate);
	void CancelInvoke(const StringName &p_method = StringName());
	bool IsInvoking(const StringName &p_method = StringName()) const;
};
