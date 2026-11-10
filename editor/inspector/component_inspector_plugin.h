/**************************************************************************/
/*  component_inspector_plugin.h                                          */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "editor/inspector/editor_inspector.h"
#include "scene/gui/box_container.h"

class Button;
class CheckBox;
class EditorSpinSlider;
class LineEdit;
class MenuButton;
class Node2D;

// Unity-style game object header: active toggle, name and tag (a global group).
class GameObjectHeader : public HBoxContainer {
	GDCLASS(GameObjectHeader, HBoxContainer);

	ObjectID node_id;
	CheckBox *active = nullptr;
	LineEdit *name_edit = nullptr;
	MenuButton *tag_button = nullptr;

	Node *_get_node() const;
	void _active_toggled(bool p_active);
	void _name_submitted(const String &p_name);
	void _name_focus_exited();
	void _tag_about_to_popup();
	void _tag_selected(int p_index);
	void _update();

protected:
	void _notification(int p_what);

public:
	void set_node(Node *p_node);
};

// Unity-style Transform section for a Node2D: local position, rotation and scale
// in Unity's 2D coordinates (Y points up, angles are counter-clockwise).
class Transform2DSection : public VBoxContainer {
	GDCLASS(Transform2DSection, VBoxContainer);

	ObjectID node_id;
	Button *fold = nullptr;
	VBoxContainer *body = nullptr;
	EditorSpinSlider *position[2] = {};
	EditorSpinSlider *rotation = nullptr;
	EditorSpinSlider *scale[2] = {};
	bool updating = false;

	static bool folded;

	Node2D *_get_node() const;
	EditorSpinSlider *_add_spin(HBoxContainer *p_row, const String &p_label, const String &p_suffix);
	void _value_changed(double p_value);
	void _toggle_folded();
	void _update_values();

protected:
	void _notification(int p_what);

public:
	void set_node(Node2D *p_node);
};

// Unity-style component list: the components of a game object (child nodes such as scripts,
// collision shapes or audio players, see Node::unity_is_component()) are edited in its Inspector,
// each one in its own foldable section, followed by an "Advanced" section.
class ComponentListEditor : public VBoxContainer {
	GDCLASS(ComponentListEditor, VBoxContainer);

	enum MenuOption {
		MENU_REMOVE,
		MENU_MOVE_UP,
		MENU_MOVE_DOWN,
		MENU_EDIT_SCRIPT,
		MENU_ATTACH_SCRIPT,
	};

	ObjectID game_object_id;
	bool built = false;
	bool refresh_queued = false;

	static HashSet<ObjectID> collapsed;
	static bool advanced_folded;

	Node *_get_game_object() const;
	EditorInspector *_get_root_inspector() const;
	EditorInspector *_create_sub_inspector(EditorInspector *p_root);
	void _build();
	void _add_component(Node *p_component, EditorInspector *p_root);
	void _add_advanced(Node *p_game_object, EditorInspector *p_root);
	void _queue_refresh();
	void _refresh();

	void _toggle_folded(Button *p_button, Control *p_body, ObjectID p_component);
	void _toggle_advanced(Button *p_button, Control *p_body);
	void _toggle_enabled(bool p_enabled, ObjectID p_component);
	void _menu_option(int p_option, ObjectID p_component);

	static String _get_enabled_property(Node *p_component, bool &r_inverted);

protected:
	void _notification(int p_what);

public:
	static String get_component_title(Node *p_component);
	void set_game_object(Node *p_node);
};

// Only used by the main Inspector (EditorInspector::set_unity_object_layout()).
class ComponentInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(ComponentInspectorPlugin, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
	virtual void parse_end(Object *p_object) override;
};
