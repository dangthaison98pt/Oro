/**************************************************************************/
/*  component_inspector_plugin.cpp                                        */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "component_inspector_plugin.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"

HashSet<ObjectID> ComponentListEditor::collapsed;
bool ComponentListEditor::advanced_folded = true;
bool Transform2DSection::folded = false;

static bool _is_game_object(const Node *p_node) {
	const Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	return p_node && edited_scene && (p_node == edited_scene || edited_scene->is_ancestor_of(p_node)) && !p_node->unity_is_component();
}

static Ref<Texture2D> _fold_icon(Control *p_control, bool p_folded) {
	return p_control->get_editor_theme_icon(p_folded ? SNAME("GuiTreeArrowRight") : SNAME("GuiTreeArrowDown"));
}

/////////////////////////////////////////////////////////////////////////////
// GameObjectHeader

Node *GameObjectHeader::_get_node() const {
	return ObjectDB::get_instance<Node>(node_id);
}

void GameObjectHeader::set_node(Node *p_node) {
	node_id = p_node->get_instance_id();

	add_theme_constant_override("separation", 6 * EDSCALE);

	active = memnew(CheckBox);
	active->set_focus_mode(FOCUS_NONE);
	active->set_tooltip_text(TTR("Active: an inactive game object is hidden and doesn't run (SetActive)."));
	active->connect(SceneStringName(toggled), callable_mp(this, &GameObjectHeader::_active_toggled));
	add_child(active);

	name_edit = memnew(LineEdit);
	name_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	name_edit->set_select_all_on_focus(true);
	name_edit->connect(SceneStringName(text_submitted), callable_mp(this, &GameObjectHeader::_name_submitted));
	name_edit->connect(SceneStringName(focus_exited), callable_mp(this, &GameObjectHeader::_name_focus_exited));
	add_child(name_edit);

	Label *tag_label = memnew(Label);
	tag_label->set_text(TTR("Tag"));
	add_child(tag_label);

	tag_button = memnew(MenuButton);
	tag_button->set_flat(false);
	tag_button->set_custom_minimum_size(Size2(90, 0) * EDSCALE);
	tag_button->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	tag_button->set_tooltip_text(TTR("Tag of the game object (CompareTag). Tags are the project's global groups."));
	tag_button->connect("about_to_popup", callable_mp(this, &GameObjectHeader::_tag_about_to_popup));
	tag_button->get_popup()->connect("index_pressed", callable_mp(this, &GameObjectHeader::_tag_selected));
	add_child(tag_button);

	_update();
}

void GameObjectHeader::_update() {
	Node *node = _get_node();
	if (!node || !active) {
		return;
	}
	active->set_pressed_no_signal(node->unity_is_active_self());
	if (!name_edit->has_focus() && name_edit->get_text() != String(node->get_name())) {
		name_edit->set_text(node->get_name());
	}
	String tag = "Untagged";
	for (const KeyValue<StringName, String> &E : ProjectSettings::get_singleton()->get_global_groups_list()) {
		if (node->is_in_group(E.key)) {
			tag = E.key;
			break;
		}
	}
	if (tag_button->get_text() != tag) {
		tag_button->set_text(tag);
	}
}

void GameObjectHeader::_active_toggled(bool p_active) {
	Node *node = _get_node();
	ERR_FAIL_NULL(node);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_active ? TTR("Activate Game Object") : TTR("Deactivate Game Object"), UndoRedo::MERGE_DISABLE, node);
	undo_redo->add_do_method(node, "SetActive", p_active);
	undo_redo->add_undo_property(node, "process_mode", node->get_process_mode());
	if (Object::cast_to<CanvasItem>(node)) {
		undo_redo->add_undo_property(node, "visible", node->get("visible"));
	}
	undo_redo->commit_action();
}

void GameObjectHeader::_name_submitted(const String &p_name) {
	Node *node = _get_node();
	ERR_FAIL_NULL(node);
	const String name = p_name.strip_edges();
	if (!name.is_empty() && name != String(node->get_name())) {
		SceneTreeDock::get_singleton()->get_tree_editor()->rename_node(node, name);
	}
	name_edit->set_text(node->get_name());
	if (name_edit->has_focus()) {
		name_edit->release_focus();
	}
}

void GameObjectHeader::_name_focus_exited() {
	_name_submitted(name_edit->get_text());
}

void GameObjectHeader::_tag_about_to_popup() {
	PopupMenu *popup = tag_button->get_popup();
	popup->clear();
	popup->add_item("Untagged");
	List<StringName> tags;
	for (const KeyValue<StringName, String> &E : ProjectSettings::get_singleton()->get_global_groups_list()) {
		tags.push_back(E.key);
	}
	tags.sort_custom<StringName::AlphCompare>();
	for (const StringName &tag : tags) {
		popup->add_item(tag);
	}
	if (tags.is_empty()) {
		popup->add_separator();
		popup->add_item(TTR("Add tags in Project Settings > Global Groups."));
		popup->set_item_disabled(-1, true);
	}
}

void GameObjectHeader::_tag_selected(int p_index) {
	Node *node = _get_node();
	ERR_FAIL_NULL(node);
	const String tag = p_index == 0 ? String() : tag_button->get_popup()->get_item_text(p_index);

	// One tag per game object, like Unity: remove the other tags (global groups).
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Tag"), UndoRedo::MERGE_DISABLE, node);
	for (const KeyValue<StringName, String> &E : ProjectSettings::get_singleton()->get_global_groups_list()) {
		if (node->is_in_group(E.key) && String(E.key) != tag) {
			undo_redo->add_do_method(node, "remove_from_group", E.key);
			undo_redo->add_undo_method(node, "add_to_group", E.key, true);
		}
	}
	if (!tag.is_empty() && !node->is_in_group(tag)) {
		undo_redo->add_do_method(node, "add_to_group", tag, true);
		undo_redo->add_undo_method(node, "remove_from_group", tag);
	}
	undo_redo->commit_action();
	_update();
}

void GameObjectHeader::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process(is_visible_in_tree());
		} break;
		case NOTIFICATION_PROCESS: {
			_update(); // Follows renames, undo and SetActive() from elsewhere.
		} break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Transform2DSection

Node2D *Transform2DSection::_get_node() const {
	return ObjectDB::get_instance<Node2D>(node_id);
}

EditorSpinSlider *Transform2DSection::_add_spin(HBoxContainer *p_row, const String &p_label, const String &p_suffix) {
	EditorSpinSlider *spin = memnew(EditorSpinSlider);
	spin->set_flat(true);
	spin->set_label(p_label);
	spin->set_suffix(p_suffix);
	spin->set_min(-1e9);
	spin->set_max(1e9);
	spin->set_step(0.001);
	spin->set_allow_greater(true);
	spin->set_allow_lesser(true);
	spin->set_hide_slider(true);
	spin->set_h_size_flags(SIZE_EXPAND_FILL);
	spin->connect(SceneStringName(value_changed), callable_mp(this, &Transform2DSection::_value_changed));
	p_row->add_child(spin);
	return spin;
}

void Transform2DSection::set_node(Node2D *p_node) {
	node_id = p_node->get_instance_id();

	fold = memnew(Button);
	fold->set_flat(true);
	fold->set_text(TTR("Transform"));
	fold->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	fold->set_focus_mode(FOCUS_NONE);
	fold->connect(SceneStringName(pressed), callable_mp(this, &Transform2DSection::_toggle_folded));
	add_child(fold);

	body = memnew(VBoxContainer);
	body->set_visible(!folded);
	add_child(body);

	const String row_names[3] = { TTR("Position"), TTR("Rotation"), TTR("Scale") };
	for (int r = 0; r < 3; r++) {
		HBoxContainer *row = memnew(HBoxContainer);
		body->add_child(row);
		Label *label = memnew(Label);
		label->set_text(row_names[r]);
		label->set_custom_minimum_size(Size2(80, 0) * EDSCALE);
		row->add_child(label);
		if (r == 0) {
			position[0] = _add_spin(row, "X", "");
			position[1] = _add_spin(row, "Y", "");
		} else if (r == 1) {
			rotation = _add_spin(row, "Z", U"°");
		} else {
			scale[0] = _add_spin(row, "X", "");
			scale[1] = _add_spin(row, "Y", "");
		}
	}
	position[0]->set_tooltip_text(TTR("Local position (Unity coordinates: Y points up)."));
	position[1]->set_tooltip_text(TTR("Local position (Unity coordinates: Y points up)."));
	rotation->set_tooltip_text(TTR("Local rotation in degrees, counter-clockwise (Unity's Z angle)."));

	_update_values();
}

void Transform2DSection::_toggle_folded() {
	folded = !folded;
	body->set_visible(!folded);
	fold->set_button_icon(_fold_icon(this, folded));
}

void Transform2DSection::_update_values() {
	Node2D *node = _get_node();
	if (!node || !position[0]) {
		return;
	}
	updating = true;
	const Vector2 pos = node->get_position();
	const double values[5] = { pos.x, -pos.y, -Math::rad_to_deg(node->get_rotation()), node->get_scale().x, node->get_scale().y };
	EditorSpinSlider *spins[5] = { position[0], position[1], rotation, scale[0], scale[1] };
	for (int i = 0; i < 5; i++) {
		// Don't fight the user while they type or drag.
		const Control *focused = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
		const bool editing = spins[i]->is_grabbing() || (focused && (focused == spins[i] || spins[i]->is_ancestor_of(focused)));
		if (!editing && !Math::is_equal_approx(spins[i]->get_value(), values[i])) {
			spins[i]->set_value_no_signal(values[i]);
		}
	}
	updating = false;
}

void Transform2DSection::_value_changed(double p_value) {
	Node2D *node = _get_node();
	if (updating || !node) {
		return;
	}
	const Vector2 new_position(position[0]->get_value(), -position[1]->get_value());
	const real_t new_rotation = Math::deg_to_rad(-rotation->get_value());
	const Vector2 new_scale(scale[0]->get_value(), scale[1]->get_value());

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Transform"), UndoRedo::MERGE_ENDS, node);
	undo_redo->add_do_property(node, "position", new_position);
	undo_redo->add_do_property(node, "rotation", new_rotation);
	undo_redo->add_do_property(node, "scale", new_scale);
	undo_redo->add_undo_property(node, "position", node->get_position());
	undo_redo->add_undo_property(node, "rotation", node->get_rotation());
	undo_redo->add_undo_property(node, "scale", node->get_scale());
	undo_redo->commit_action();
}

void Transform2DSection::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			if (!fold) {
				break;
			}
			fold->set_button_icon(_fold_icon(this, folded));
			fold->add_theme_font_override(SNAME("font"), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
			const Color x_color = get_theme_color(SNAME("property_color_x"), EditorStringName(Editor));
			const Color y_color = get_theme_color(SNAME("property_color_y"), EditorStringName(Editor));
			const Color z_color = get_theme_color(SNAME("property_color_z"), EditorStringName(Editor));
			position[0]->add_theme_color_override("label_color", x_color);
			position[1]->add_theme_color_override("label_color", y_color);
			rotation->add_theme_color_override("label_color", z_color);
			scale[0]->add_theme_color_override("label_color", x_color);
			scale[1]->add_theme_color_override("label_color", y_color);
		} break;
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process(is_visible_in_tree());
		} break;
		case NOTIFICATION_PROCESS: {
			_update_values(); // Follows the 2D view, undo and scripts.
		} break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// ComponentListEditor

Node *ComponentListEditor::_get_game_object() const {
	return ObjectDB::get_instance<Node>(game_object_id);
}

EditorInspector *ComponentListEditor::_get_root_inspector() const {
	for (Node *parent = get_parent(); parent; parent = parent->get_parent()) {
		EditorInspector *inspector = Object::cast_to<EditorInspector>(parent);
		if (inspector) {
			return inspector->get_root_inspector();
		}
	}
	return nullptr;
}

String ComponentListEditor::get_component_title(Node *p_component) {
	const Ref<Script> script = p_component->get_script();
	if (script.is_valid()) {
		String name = script->get_global_name();
		if (name.is_empty()) {
			name = script->get_path().get_file().get_basename().to_pascal_case();
		}
		if (!name.is_empty()) {
			return name + " (Script)";
		}
	}
	if (p_component->is_class("MonoBehaviour")) {
		return "MonoBehaviour (No Script)";
	}
	return p_component->get_class();
}

String ComponentListEditor::_get_enabled_property(Node *p_component, bool &r_inverted) {
	r_inverted = false;
	if (p_component->is_class("MonoBehaviour")) {
		return "enabled";
	}
	if (p_component->is_class("CollisionShape2D") || p_component->is_class("CollisionPolygon2D")) {
		r_inverted = true;
		return "disabled";
	}
	if (p_component->is_class("CanvasItem")) {
		return "visible";
	}
	return String();
}

void ComponentListEditor::set_game_object(Node *p_node) {
	game_object_id = p_node ? p_node->get_instance_id() : ObjectID();
}

EditorInspector *ComponentListEditor::_create_sub_inspector(EditorInspector *p_root) {
	EditorInspector *inspector = memnew(EditorInspector);
	inspector->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	inspector->set_use_doc_hints(true);
	if (p_root) {
		inspector->set_root_inspector(p_root);
		inspector->set_use_folding(p_root->is_using_folding());
	}
	inspector->set_property_name_style(InspectorDock::get_singleton()->get_property_name_style());
	inspector->set_draw_focus_border(false);
	inspector->set_focus_mode(FOCUS_NONE);
	return inspector;
}

void ComponentListEditor::_build() {
	built = true;
	Node *game_object = _get_game_object();
	if (!game_object) {
		return;
	}
	EditorInspector *root = _get_root_inspector();

	for (int i = 0; i < game_object->get_child_count(false); i++) {
		Node *child = game_object->get_child(i, false);
		if (child->unity_is_component()) {
			_add_component(child, root);
		}
	}
	// Orostride: no Advanced section. Node settings (process mode, priorities, physics interpolation,
	// auto translate, skew...) are set from code, like in Unity.

	// Keep the list in sync when components are added, removed or reordered.
	game_object->connect(SNAME("child_order_changed"), callable_mp(this, &ComponentListEditor::_queue_refresh));
}

void ComponentListEditor::_add_component(Node *p_component, EditorInspector *p_root) {
	const ObjectID component_id = p_component->get_instance_id();
	const bool folded = collapsed.has(component_id);

	add_child(memnew(HSeparator));

	// Header: fold arrow, enabled toggle, icon + name, menu.
	HBoxContainer *header = memnew(HBoxContainer);
	header->add_theme_constant_override("separation", 2 * EDSCALE);
	add_child(header);

	Button *fold = memnew(Button);
	fold->set_flat(true);
	fold->set_focus_mode(FOCUS_NONE);
	fold->set_button_icon(_fold_icon(this, folded));
	header->add_child(fold);

	bool inverted = false;
	const String enabled_property = _get_enabled_property(p_component, inverted);
	if (!enabled_property.is_empty()) {
		CheckBox *enabled = memnew(CheckBox);
		enabled->set_focus_mode(FOCUS_NONE);
		enabled->set_tooltip_text(TTR("Enable or disable this component."));
		const bool value = p_component->get(enabled_property);
		enabled->set_pressed_no_signal(inverted ? !value : value);
		enabled->connect(SceneStringName(toggled), callable_mp(this, &ComponentListEditor::_toggle_enabled).bind(component_id));
		header->add_child(enabled);
	}

	Button *title = memnew(Button);
	title->set_flat(true);
	title->set_h_size_flags(SIZE_EXPAND_FILL);
	title->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	title->set_text(get_component_title(p_component));
	title->set_button_icon(Ref<Script>(p_component->get_script()).is_valid() ? get_editor_theme_icon(SNAME("Script")) : EditorNode::get_singleton()->get_object_icon(p_component, "Node"));
	title->set_tooltip_text(vformat(TTR("Component node: %s"), p_component->get_name()));
	title->add_theme_font_override(SNAME("font"), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	header->add_child(title);

	MenuButton *menu = memnew(MenuButton);
	menu->set_flat(true);
	menu->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
	menu->set_tooltip_text(TTR("Component options."));
	PopupMenu *popup = menu->get_popup();
	popup->add_icon_item(get_editor_theme_icon(SNAME("Remove")), TTR("Remove Component"), MENU_REMOVE);
	popup->add_separator();
	popup->add_icon_item(get_editor_theme_icon(SNAME("MoveUp")), TTR("Move Up"), MENU_MOVE_UP);
	popup->add_icon_item(get_editor_theme_icon(SNAME("MoveDown")), TTR("Move Down"), MENU_MOVE_DOWN);
	popup->add_separator();
	if (Ref<Script>(p_component->get_script()).is_valid()) {
		popup->add_icon_item(get_editor_theme_icon(SNAME("Script")), TTR("Edit Script"), MENU_EDIT_SCRIPT);
	} else {
		popup->add_icon_item(get_editor_theme_icon(SNAME("ScriptCreate")), TTR("Attach Script..."), MENU_ATTACH_SCRIPT);
	}
	popup->connect(SceneStringName(id_pressed), callable_mp(this, &ComponentListEditor::_menu_option).bind(component_id));
	header->add_child(menu);

	// Body: the component's own properties.
	MarginContainer *body = memnew(MarginContainer);
	body->add_theme_constant_override("margin_left", 8 * EDSCALE);
	body->set_visible(!folded);
	add_child(body);

	EditorInspector *inspector = _create_sub_inspector(p_root);
	// Like Unity, a component shows only its own settings: no transform, visibility or node settings.
	// MonoBehaviour's "enabled" is the checkbox in the header.
	// Renderers keep their render settings (like a SpriteRenderer); "visible" is the header checkbox.
	if (ComponentInspectorPlugin::is_renderer(p_component)) {
		inspector->set_hidden_property_classes({ SNAME("Node"), SNAME("Node2D"), SNAME("MonoBehaviour") });
		inspector->set_hidden_property_names({ SNAME("visible") });
	} else {
		inspector->set_hidden_property_classes({ SNAME("Node"), SNAME("CanvasItem"), SNAME("Node2D"), SNAME("MonoBehaviour") });
	}
	body->add_child(inspector);
	inspector->edit(p_component);

	title->connect(SceneStringName(pressed), callable_mp(this, &ComponentListEditor::_toggle_folded).bind(fold, body, component_id));
	fold->connect(SceneStringName(pressed), callable_mp(this, &ComponentListEditor::_toggle_folded).bind(fold, body, component_id));
}

void ComponentListEditor::_add_advanced(Node *p_game_object, EditorInspector *p_root) {
	// Node and skew settings, which are rarely needed. (Group color and sorting are in the
	// Sorting Group component, added only where needed.)
	add_child(memnew(HSeparator));

	Button *advanced = memnew(Button);
	advanced->set_flat(true);
	advanced->set_text(TTR("Advanced"));
	advanced->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	advanced->set_focus_mode(FOCUS_NONE);
	advanced->set_tooltip_text(TTR("Process, physics interpolation and other node settings."));
	advanced->set_button_icon(_fold_icon(this, advanced_folded));
	advanced->add_theme_font_override(SNAME("font"), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
	add_child(advanced);

	MarginContainer *body = memnew(MarginContainer);
	body->add_theme_constant_override("margin_left", 8 * EDSCALE);
	body->set_visible(!advanced_folded);
	add_child(body);

	EditorInspector *inspector = _create_sub_inspector(p_root);
	// Render settings are in the renderer's own section or in the Sorting Group section.
	inspector->set_shown_property_classes({ SNAME("Node"), SNAME("Node2D") });
	inspector->set_hidden_property_names({ SNAME("position"), SNAME("rotation"), SNAME("scale"), SNAME("skew") }); // Transform section; skew is code-only (Unity has none).
	inspector->set_hide_metadata(false);
	body->add_child(inspector);
	inspector->edit(p_game_object);

	advanced->connect(SceneStringName(pressed), callable_mp(this, &ComponentListEditor::_toggle_advanced).bind(advanced, body));
}

void ComponentListEditor::_toggle_folded(Button *p_button, Control *p_body, ObjectID p_component) {
	const bool folded = p_body->is_visible();
	p_body->set_visible(!folded);
	p_button->set_button_icon(_fold_icon(this, folded));
	if (folded) {
		collapsed.insert(p_component);
	} else {
		collapsed.erase(p_component);
	}
}

void ComponentListEditor::_toggle_advanced(Button *p_button, Control *p_body) {
	advanced_folded = p_body->is_visible();
	p_body->set_visible(!advanced_folded);
	p_button->set_button_icon(_fold_icon(this, advanced_folded));
}

void ComponentListEditor::_toggle_enabled(bool p_enabled, ObjectID p_component) {
	Node *component = ObjectDB::get_instance<Node>(p_component);
	ERR_FAIL_NULL(component);
	bool inverted = false;
	const String property = _get_enabled_property(component, inverted);
	ERR_FAIL_COND(property.is_empty());

	const Variant old_value = component->get(property);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_enabled ? TTR("Enable Component") : TTR("Disable Component"), UndoRedo::MERGE_DISABLE, component);
	undo_redo->add_do_property(component, property, inverted ? !p_enabled : p_enabled);
	undo_redo->add_undo_property(component, property, old_value);
	undo_redo->commit_action();
}

void ComponentListEditor::_menu_option(int p_option, ObjectID p_component) {
	Node *component = ObjectDB::get_instance<Node>(p_component);
	Node *game_object = _get_game_object();
	ERR_FAIL_NULL(component);
	ERR_FAIL_NULL(game_object);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();

	switch (p_option) {
		case MENU_REMOVE: {
			const int index = component->get_index(false);
			undo_redo->create_action(TTR("Remove Component"), UndoRedo::MERGE_DISABLE, game_object);
			undo_redo->add_do_method(game_object, "remove_child", component);
			undo_redo->add_undo_method(game_object, "add_child", component, true);
			undo_redo->add_undo_method(game_object, "move_child", component, index);
			undo_redo->add_undo_method(component, "set_owner", component->get_owner());
			undo_redo->add_undo_reference(component);
			undo_redo->commit_action();
		} break;
		case MENU_MOVE_UP:
		case MENU_MOVE_DOWN: {
			// Move past the neighboring component, skipping game objects in between.
			const int index = component->get_index(false);
			const int step = p_option == MENU_MOVE_UP ? -1 : 1;
			int target = -1;
			for (int i = index + step; i >= 0 && i < game_object->get_child_count(false); i += step) {
				if (game_object->get_child(i, false)->unity_is_component()) {
					target = i;
					break;
				}
			}
			if (target < 0) {
				break;
			}
			undo_redo->create_action(TTR("Move Component"), UndoRedo::MERGE_DISABLE, game_object);
			undo_redo->add_do_method(game_object, "move_child", component, target);
			undo_redo->add_undo_method(game_object, "move_child", component, index);
			undo_redo->commit_action();
		} break;
		case MENU_ATTACH_SCRIPT: {
			SceneTreeDock::get_singleton()->attach_script_to_component(component);
		} break;
		case MENU_EDIT_SCRIPT: {
			const Ref<Script> script = component->get_script();
			if (script.is_valid()) {
				EditorNode::get_singleton()->edit_resource(script);
			}
		} break;
	}
}

void ComponentListEditor::_queue_refresh() {
	if (refresh_queued) {
		return;
	}
	refresh_queued = true;
	callable_mp(this, &ComponentListEditor::_refresh).call_deferred();
}

void ComponentListEditor::_refresh() {
	refresh_queued = false;
	EditorInspector *root = _get_root_inspector();
	if (root) {
		// Rebuilds the whole Inspector, including this list.
		root->update_tree();
	}
}

void ComponentListEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE && !built) {
		// Deferred: the sub-inspectors must not be created while the Inspector is still
		// running its plugins (they are shared between inspectors).
		built = true;
		callable_mp(this, &ComponentListEditor::_build).call_deferred();
	}
}

/////////////////////////////////////////////////////////////////////////////
// ComponentInspectorPlugin

bool ComponentInspectorPlugin::is_renderer(const Object *p_object) {
	if (!Object::cast_to<CanvasItem>(p_object) || p_object->get_class() == "Node2D") {
		return false;
	}
	static const char *non_drawing[] = {
		"CollisionObject2D", "CollisionShape2D", "CollisionPolygon2D", "Camera2D", "Marker2D", "RemoteTransform2D",
		"AudioStreamPlayer2D", "AudioListener2D", "VisibleOnScreenNotifier2D", "NavigationRegion2D", "NavigationLink2D",
		"NavigationObstacle2D", "NavigationAgent2D", "Path2D", "PathFollow2D", "Skeleton2D", "Bone2D", "Joint2D",
		"RayCast2D", "ShapeCast2D", "BackBufferCopy", "CanvasModulate", "Parallax2D", "TouchScreenButton",
	};
	for (const char *class_name : non_drawing) {
		if (p_object->is_class(class_name)) {
			return false;
		}
	}
	return true;
}

bool ComponentInspectorPlugin::can_handle(Object *p_object) {
	// Every game object of the edited scene gets the header and the list, even an empty one,
	// so that components added with "Add Component" show up right away.
	return _is_game_object(Object::cast_to<Node>(p_object));
}

void ComponentInspectorPlugin::parse_begin(Object *p_object) {
	Node *node = Object::cast_to<Node>(p_object);

	GameObjectHeader *header = memnew(GameObjectHeader);
	header->set_node(node);
	add_custom_control(header);

	Node2D *node_2d = Object::cast_to<Node2D>(node);
	if (node_2d) {
		Transform2DSection *transform = memnew(Transform2DSection);
		transform->set_node(node_2d);
		add_custom_control(transform);
	}
}

void ComponentInspectorPlugin::parse_end(Object *p_object) {
	ComponentListEditor *list = memnew(ComponentListEditor);
	list->set_game_object(Object::cast_to<Node>(p_object));
	add_custom_control(list);
}
