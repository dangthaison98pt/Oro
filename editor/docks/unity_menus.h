/**************************************************************************/
/*  unity_menus.h                                                         */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "scene/gui/dialogs.h"
#include "scene/gui/popup_menu.h"

class LineEdit;
class Tree;

// Unity-style "Create" menu of the Hierarchy: Create Empty, 2D Object, Camera, Audio Source, UI...
// Game objects are Node2Ds; their components are child nodes.
class GameObjectCreateMenu : public PopupMenu {
	GDCLASS(GameObjectCreateMenu, PopupMenu);

	enum {
		CREATE_EMPTY,
		CREATE_EMPTY_CHILD,
		CREATE_SQUARE = 10,
		CREATE_CIRCLE,
		CREATE_SPRITE,
		CREATE_ANIMATED_SPRITE,
		CREATE_TILEMAP,
		CREATE_LINE,
		CREATE_POLYGON,
		CREATE_CAMERA = 20,
		CREATE_AUDIO_SOURCE,
		CREATE_LIGHT,
		CREATE_PARTICLES,
		CREATE_UI_CANVAS = 30,
		CREATE_UI_TEXT,
		CREATE_UI_BUTTON,
		CREATE_UI_IMAGE,
		CREATE_UI_PANEL,
		CREATE_OTHER = 40,
	};

	bool as_child = false;
	PopupMenu *object_2d_menu = nullptr;
	PopupMenu *ui_menu = nullptr;

	void _id_pressed(int p_id);

protected:
	void _notification(int p_what);

public:
	// As child: new objects go under the selected object (context menu); otherwise at the scene root.
	void set_as_child(bool p_as_child) { as_child = p_as_child; }

	GameObjectCreateMenu();
};

// Unity-style "Add Component" window: components by category (Scripts, Rendering, Physics 2D...),
// with search. Physics bodies change the game object's type, like adding a Rigidbody2D in Unity.
class AddComponentDialog : public ConfirmationDialog {
	GDCLASS(AddComponentDialog, ConfirmationDialog);

	ObjectID target_id;
	LineEdit *search = nullptr;
	Tree *tree = nullptr;
	AcceptDialog *error_dialog = nullptr;

	void _update_list();
	void _search_changed(const String &p_text);
	void _search_gui_input(const Ref<InputEvent> &p_event);
	void _item_activated();
	void _item_selected();
	void _apply(const String &p_entry);
	void _show_error(const String &p_text);

protected:
	void _notification(int p_what);
	virtual void ok_pressed() override;

public:
	void popup_for(Node *p_game_object);

	AddComponentDialog();
};

// Shared helpers.
class GameObjectUtils {
public:
	// Adds p_object (and its children) under p_parent with undo, then selects it.
	static void add_object(Node *p_parent, Node *p_object, const String &p_action_name);
	static Ref<Texture2D> make_shape_texture(bool p_circle);
};
