/**************************************************************************/
/*  unity_transform.h                                                     */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"

class Node;
class Node2D;
class Control;

// Unity-style Transform of a game object (a Node2D or a Control).
// It uses Unity's 2D conventions: Y points up, angles are in degrees and counter-clockwise.
class Transform : public RefCounted {
	GDCLASS(Transform, RefCounted);

	ObjectID target_id;

	Node *_get_target() const;
	Node2D *_get_node2d() const;
	Control *_get_control() const;

	static Vector2 _to_unity(const Vector2 &p_godot) { return Vector2(p_godot.x, -p_godot.y); }
	static Vector2 _to_godot(const Vector2 &p_unity) { return Vector2(p_unity.x, -p_unity.y); }
	static Vector2 _variant_to_vector2(const Variant &p_value);

protected:
	static void _bind_methods();

public:
	static Ref<Transform> for_node(Node *p_node);
	void set_target(Node *p_node);

	Node *get_game_object() const;
	String get_name() const;

	Vector2 get_position() const;
	void set_position(const Variant &p_position);
	Vector2 get_local_position() const;
	void set_local_position(const Variant &p_position);

	Vector3 get_euler_angles() const;
	void set_euler_angles(const Variant &p_angles);
	Vector3 get_local_euler_angles() const;
	void set_local_euler_angles(const Variant &p_angles);

	Vector2 get_local_scale() const;
	void set_local_scale(const Variant &p_scale);

	Vector2 get_right() const;
	Vector2 get_up() const;

	Ref<Transform> get_parent() const;
	int get_child_count() const;

	void Translate(const Variant &p_translation, bool p_world_space = false);
	void Rotate(const Variant &p_angle);
	Ref<Transform> GetChild(int p_index) const;
	Ref<Transform> Find(const String &p_name) const;
	void SetParent(Object *p_parent, bool p_world_position_stays = true);
};
