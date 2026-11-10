/**************************************************************************/
/*  unity_transform.cpp                                                   */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "unity_transform.h"

#include "core/object/class_db.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"

Ref<Transform> Transform::for_node(Node *p_node) {
	Ref<Transform> transform;
	transform.instantiate();
	transform->set_target(p_node);
	return transform;
}

void Transform::set_target(Node *p_node) {
	target_id = p_node ? p_node->get_instance_id() : ObjectID();
}

Node *Transform::_get_target() const {
	return ObjectDB::get_instance<Node>(target_id);
}

Node2D *Transform::_get_node2d() const {
	return Object::cast_to<Node2D>(_get_target());
}

Control *Transform::_get_control() const {
	return Object::cast_to<Control>(_get_target());
}

Vector2 Transform::_variant_to_vector2(const Variant &p_value) {
	// Unity code often passes a Vector3 (z is ignored in 2D).
	switch (p_value.get_type()) {
		case Variant::VECTOR2:
		case Variant::VECTOR2I:
			return p_value;
		case Variant::VECTOR3: {
			const Vector3 v = p_value;
			return Vector2(v.x, v.y);
		}
		case Variant::VECTOR3I: {
			const Vector3i v = p_value;
			return Vector2(v.x, v.y);
		}
		default:
			ERR_FAIL_V_MSG(Vector2(), "Expected a Vector2 or a Vector3.");
	}
}

Node *Transform::get_game_object() const {
	return _get_target();
}

String Transform::get_name() const {
	Node *node = _get_target();
	return node ? String(node->get_name()) : String();
}

#define TRANSFORM_FAIL_V(m_ret) ERR_FAIL_V_MSG(m_ret, "This Transform's game object is not a Node2D or a Control (or it was freed).")
#define TRANSFORM_FAIL() ERR_FAIL_MSG("This Transform's game object is not a Node2D or a Control (or it was freed).")

Vector2 Transform::get_position() const {
	if (Node2D *n = _get_node2d()) {
		return _to_unity(n->get_global_position());
	}
	if (Control *c = _get_control()) {
		return _to_unity(c->get_global_position());
	}
	TRANSFORM_FAIL_V(Vector2());
}

void Transform::set_position(const Variant &p_position) {
	const Vector2 position = _to_godot(_variant_to_vector2(p_position));
	if (Node2D *n = _get_node2d()) {
		n->set_global_position(position);
	} else if (Control *c = _get_control()) {
		c->set_global_position(position);
	} else {
		TRANSFORM_FAIL();
	}
}

Vector2 Transform::get_local_position() const {
	if (Node2D *n = _get_node2d()) {
		return _to_unity(n->get_position());
	}
	if (Control *c = _get_control()) {
		return _to_unity(c->get_position());
	}
	TRANSFORM_FAIL_V(Vector2());
}

void Transform::set_local_position(const Variant &p_position) {
	const Vector2 position = _to_godot(_variant_to_vector2(p_position));
	if (Node2D *n = _get_node2d()) {
		n->set_position(position);
	} else if (Control *c = _get_control()) {
		c->set_position(position);
	} else {
		TRANSFORM_FAIL();
	}
}

static double _angle_from_variant(const Variant &p_angle) {
	// A number (z angle) or a Vector3 of Euler angles (only z is used in 2D).
	if (p_angle.get_type() == Variant::VECTOR3 || p_angle.get_type() == Variant::VECTOR3I) {
		const Vector3 v = p_angle;
		return v.z;
	}
	return p_angle;
}

Vector3 Transform::get_euler_angles() const {
	if (Node2D *n = _get_node2d()) {
		return Vector3(0, 0, -Math::rad_to_deg(n->get_global_rotation()));
	}
	if (Control *c = _get_control()) {
		return Vector3(0, 0, -Math::rad_to_deg(c->get_rotation()));
	}
	TRANSFORM_FAIL_V(Vector3());
}

void Transform::set_euler_angles(const Variant &p_angles) {
	const double rotation = -Math::deg_to_rad(_angle_from_variant(p_angles));
	if (Node2D *n = _get_node2d()) {
		n->set_global_rotation(rotation);
	} else if (Control *c = _get_control()) {
		c->set_rotation(rotation);
	} else {
		TRANSFORM_FAIL();
	}
}

Vector3 Transform::get_local_euler_angles() const {
	if (Node2D *n = _get_node2d()) {
		return Vector3(0, 0, -Math::rad_to_deg(n->get_rotation()));
	}
	if (Control *c = _get_control()) {
		return Vector3(0, 0, -Math::rad_to_deg(c->get_rotation()));
	}
	TRANSFORM_FAIL_V(Vector3());
}

void Transform::set_local_euler_angles(const Variant &p_angles) {
	const double rotation = -Math::deg_to_rad(_angle_from_variant(p_angles));
	if (Node2D *n = _get_node2d()) {
		n->set_rotation(rotation);
	} else if (Control *c = _get_control()) {
		c->set_rotation(rotation);
	} else {
		TRANSFORM_FAIL();
	}
}

Vector2 Transform::get_local_scale() const {
	if (Node2D *n = _get_node2d()) {
		return n->get_scale();
	}
	if (Control *c = _get_control()) {
		return c->get_scale();
	}
	TRANSFORM_FAIL_V(Vector2(1, 1));
}

void Transform::set_local_scale(const Variant &p_scale) {
	const Vector2 scale = _variant_to_vector2(p_scale);
	if (Node2D *n = _get_node2d()) {
		n->set_scale(scale);
	} else if (Control *c = _get_control()) {
		c->set_scale(scale);
	} else {
		TRANSFORM_FAIL();
	}
}

Vector2 Transform::get_right() const {
	const double angle = Math::deg_to_rad(get_euler_angles().z);
	return Vector2(Math::cos(angle), Math::sin(angle));
}

Vector2 Transform::get_up() const {
	const double angle = Math::deg_to_rad(get_euler_angles().z);
	return Vector2(-Math::sin(angle), Math::cos(angle));
}

Ref<Transform> Transform::get_parent() const {
	Node *node = _get_target();
	if (!node || !node->get_parent()) {
		return Ref<Transform>();
	}
	return for_node(node->get_parent());
}

int Transform::get_child_count() const {
	Node *node = _get_target();
	return node ? node->get_child_count() : 0;
}

void Transform::Translate(const Variant &p_translation, bool p_world_space) {
	const Vector2 delta = _variant_to_vector2(p_translation);
	if (p_world_space) {
		set_position(get_position() + delta);
	} else {
		// Like Unity's default (Space.Self): move along the object's own axes.
		set_position(get_position() + get_right() * delta.x + get_up() * delta.y);
	}
}

void Transform::Rotate(const Variant &p_angle) {
	Vector3 angles = get_local_euler_angles();
	angles.z += _angle_from_variant(p_angle);
	set_local_euler_angles(angles);
}

Ref<Transform> Transform::GetChild(int p_index) const {
	Node *node = _get_target();
	ERR_FAIL_NULL_V(node, Ref<Transform>());
	ERR_FAIL_INDEX_V(p_index, node->get_child_count(), Ref<Transform>());
	return for_node(node->get_child(p_index));
}

Ref<Transform> Transform::Find(const String &p_name) const {
	Node *node = _get_target();
	ERR_FAIL_NULL_V(node, Ref<Transform>());
	Node *found = node->get_node_or_null(NodePath(p_name));
	return found ? for_node(found) : Ref<Transform>();
}

void Transform::SetParent(Object *p_parent, bool p_world_position_stays) {
	Node *node = _get_target();
	ERR_FAIL_NULL(node);
	Node *new_parent = Object::cast_to<Node>(p_parent);
	Transform *parent_transform = Object::cast_to<Transform>(p_parent);
	if (parent_transform) {
		new_parent = parent_transform->get_game_object();
	}
	ERR_FAIL_NULL_MSG(new_parent, "SetParent() expects a Transform or a Node.");
	node->reparent(new_parent, p_world_position_stays);
}

void Transform::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_game_object"), &Transform::get_game_object);
	ClassDB::bind_method(D_METHOD("get_name"), &Transform::get_name);
	ClassDB::bind_method(D_METHOD("get_position"), &Transform::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &Transform::set_position);
	ClassDB::bind_method(D_METHOD("get_local_position"), &Transform::get_local_position);
	ClassDB::bind_method(D_METHOD("set_local_position", "position"), &Transform::set_local_position);
	ClassDB::bind_method(D_METHOD("get_euler_angles"), &Transform::get_euler_angles);
	ClassDB::bind_method(D_METHOD("set_euler_angles", "angles"), &Transform::set_euler_angles);
	ClassDB::bind_method(D_METHOD("get_local_euler_angles"), &Transform::get_local_euler_angles);
	ClassDB::bind_method(D_METHOD("set_local_euler_angles", "angles"), &Transform::set_local_euler_angles);
	ClassDB::bind_method(D_METHOD("get_local_scale"), &Transform::get_local_scale);
	ClassDB::bind_method(D_METHOD("set_local_scale", "scale"), &Transform::set_local_scale);
	ClassDB::bind_method(D_METHOD("get_right"), &Transform::get_right);
	ClassDB::bind_method(D_METHOD("get_up"), &Transform::get_up);
	ClassDB::bind_method(D_METHOD("get_parent"), &Transform::get_parent);
	ClassDB::bind_method(D_METHOD("get_child_count"), &Transform::get_child_count);

	ClassDB::bind_method(D_METHOD("Translate", "translation", "world_space"), &Transform::Translate, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("Rotate", "angle"), &Transform::Rotate);
	ClassDB::bind_method(D_METHOD("GetChild", "index"), &Transform::GetChild);
	ClassDB::bind_method(D_METHOD("Find", "name"), &Transform::Find);
	ClassDB::bind_method(D_METHOD("SetParent", "parent", "world_position_stays"), &Transform::SetParent, DEFVAL(true));

	// Unity property names (camelCase on purpose).
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gameObject", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_game_object");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_position", "get_position");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "localPosition", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_local_position", "get_local_position");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "eulerAngles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_euler_angles", "get_euler_angles");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "localEulerAngles", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_local_euler_angles", "get_local_euler_angles");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "localScale", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_local_scale", "get_local_scale");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "right", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_right");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "up", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_up");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parent", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_parent");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "childCount", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_child_count");
}
