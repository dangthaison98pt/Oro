/**************************************************************************/
/*  unity_menus.cpp                                                       */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "unity_menus.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/2d/camera_2d.h"
#include "scene/2d/cpu_particles_2d.h"
#include "scene/2d/light_2d.h"
#include "scene/2d/line_2d.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/2d/physics/collision_polygon_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/main/canvas_layer.h"
#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/gradient_texture.h"

/////////////////////////////////////////////////////////////////////////////
// GameObjectUtils

static void _collect_descendants(Node *p_node, LocalVector<Node *> &r_nodes) {
	for (int i = 0; i < p_node->get_child_count(false); i++) {
		Node *child = p_node->get_child(i, false);
		r_nodes.push_back(child);
		_collect_descendants(child, r_nodes);
	}
}

void GameObjectUtils::add_object(Node *p_parent, Node *p_object, const String &p_action_name) {
	EditorNode *editor = EditorNode::get_singleton();
	SceneTreeDock *dock = SceneTreeDock::get_singleton();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	Node *edited_scene = editor->get_edited_scene();

	LocalVector<Node *> descendants;
	_collect_descendants(p_object, descendants);

	undo_redo->create_action_for_history(p_action_name, EditorNode::get_editor_data().get_current_edited_scene_history_id());
	if (!edited_scene) {
		// No scene yet: the new object becomes the scene's root.
		for (Node *node : descendants) {
			node->set_owner(p_object);
		}
		undo_redo->add_do_method(editor, "set_edited_scene", p_object);
		undo_redo->add_do_reference(p_object);
		undo_redo->add_undo_method(editor, "set_edited_scene", (Object *)nullptr);
	} else {
		Node *parent = p_parent ? p_parent : edited_scene;
		p_object->set_name(parent->validate_child_name(p_object));
		undo_redo->add_do_method(parent, "add_child", p_object, true);
		undo_redo->add_do_method(p_object, "set_owner", edited_scene);
		for (Node *node : descendants) {
			undo_redo->add_do_method(node, "set_owner", edited_scene);
		}
		undo_redo->add_do_reference(p_object);
		undo_redo->add_undo_method(parent, "remove_child", p_object);
	}
	undo_redo->add_do_method(dock, "_post_do_create", p_object);
	undo_redo->commit_action();
}

Ref<Texture2D> GameObjectUtils::make_shape_texture(bool p_circle) {
	Ref<Gradient> gradient;
	gradient.instantiate();
	gradient->set_offsets({ 0.0, 1.0 });
	Ref<GradientTexture2D> texture;
	texture.instantiate();
	texture->set_width(64);
	texture->set_height(64);
	if (p_circle) {
		gradient->set_colors({ Color(1, 1, 1, 1), Color(1, 1, 1, 0) });
		gradient->set_interpolation_mode(Gradient::GRADIENT_INTERPOLATE_CONSTANT);
		texture->set_fill(GradientTexture2D::FILL_RADIAL);
		texture->set_fill_from(Vector2(0.5, 0.5));
		texture->set_fill_to(Vector2(1.0, 0.5));
	} else {
		gradient->set_colors({ Color(1, 1, 1, 1), Color(1, 1, 1, 1) });
	}
	texture->set_gradient(gradient);
	return texture;
}

static Node2D *_make_game_object(const String &p_name) {
	Node2D *object = memnew(Node2D);
	object->set_name(p_name);
	return object;
}

static Node *_add_component_node(Node *p_object, Node *p_component, const String &p_name) {
	p_component->set_name(p_name);
	p_component->set_meta(SNAME("_component"), true);
	p_object->add_child(p_component);
	return p_component;
}

/////////////////////////////////////////////////////////////////////////////
// GameObjectCreateMenu

void GameObjectCreateMenu::_id_pressed(int p_id) {
	EditorNode *editor = EditorNode::get_singleton();
	Node *edited_scene = editor->get_edited_scene();
	Node *parent = edited_scene;
	if (as_child || p_id == CREATE_EMPTY_CHILD) {
		const List<Node *> &selection = editor->get_editor_selection()->get_top_selected_node_list();
		if (!selection.is_empty()) {
			parent = selection.front()->get();
		}
	}

	Node *object = nullptr;
	switch (p_id) {
		case CREATE_EMPTY:
		case CREATE_EMPTY_CHILD: {
			object = _make_game_object("GameObject");
		} break;
		case CREATE_SQUARE:
		case CREATE_CIRCLE: {
			object = _make_game_object(p_id == CREATE_SQUARE ? "Square" : "Circle");
			Sprite2D *sprite = memnew(Sprite2D);
			sprite->set_texture(GameObjectUtils::make_shape_texture(p_id == CREATE_CIRCLE));
			_add_component_node(object, sprite, "SpriteRenderer");
		} break;
		case CREATE_SPRITE: {
			object = _make_game_object("Sprite");
			_add_component_node(object, memnew(Sprite2D), "SpriteRenderer");
		} break;
		case CREATE_ANIMATED_SPRITE: {
			object = _make_game_object("Animated Sprite");
			_add_component_node(object, memnew(AnimatedSprite2D), "AnimatedSprite");
		} break;
		case CREATE_TILEMAP: {
			object = memnew(TileMapLayer);
			object->set_name("Tilemap");
		} break;
		case CREATE_LINE: {
			Line2D *line = memnew(Line2D);
			line->set_name("Line");
			line->set_points({ Vector2(0, 0), Vector2(100, 0) });
			object = line;
		} break;
		case CREATE_POLYGON: {
			Polygon2D *polygon = memnew(Polygon2D);
			polygon->set_name("Polygon");
			polygon->set_polygon({ Vector2(-32, -32), Vector2(32, -32), Vector2(32, 32), Vector2(-32, 32) });
			object = polygon;
		} break;
		case CREATE_CAMERA: {
			object = memnew(Camera2D);
			object->set_name("Camera");
		} break;
		case CREATE_AUDIO_SOURCE: {
			object = _make_game_object("Audio Source");
			AudioStreamPlayer2D *player = memnew(AudioStreamPlayer2D);
			player->set_name("AudioSource");
			object->add_child(player);
		} break;
		case CREATE_LIGHT: {
			PointLight2D *light = memnew(PointLight2D);
			light->set_name("Light 2D");
			Ref<Gradient> gradient;
			gradient.instantiate();
			gradient->set_offsets({ 0.0, 1.0 });
			gradient->set_colors({ Color(1, 1, 1, 1), Color(1, 1, 1, 0) });
			Ref<GradientTexture2D> texture;
			texture.instantiate();
			texture->set_width(256);
			texture->set_height(256);
			texture->set_fill(GradientTexture2D::FILL_RADIAL);
			texture->set_fill_from(Vector2(0.5, 0.5));
			texture->set_fill_to(Vector2(1.0, 0.5));
			texture->set_gradient(gradient);
			light->set_texture(texture);
			object = light;
		} break;
		case CREATE_PARTICLES: {
			object = memnew(CPUParticles2D);
			object->set_name("Particle System");
		} break;
		case CREATE_UI_CANVAS:
		case CREATE_UI_TEXT:
		case CREATE_UI_BUTTON:
		case CREATE_UI_IMAGE:
		case CREATE_UI_PANEL: {
			Node *element = nullptr;
			if (p_id == CREATE_UI_CANVAS) {
				element = memnew(CanvasLayer);
				element->set_name("Canvas");
			} else if (p_id == CREATE_UI_TEXT) {
				Label *label = memnew(Label);
				label->set_name("Text");
				label->set_text("New Text");
				element = label;
			} else if (p_id == CREATE_UI_BUTTON) {
				Button *button = memnew(Button);
				button->set_name("Button");
				button->set_text("Button");
				element = button;
			} else if (p_id == CREATE_UI_IMAGE) {
				TextureRect *image = memnew(TextureRect);
				image->set_name("Image");
				image->set_custom_minimum_size(Size2(100, 100));
				element = image;
			} else {
				Panel *ui_panel = memnew(Panel);
				ui_panel->set_name("Panel");
				ui_panel->set_custom_minimum_size(Size2(200, 100));
				element = ui_panel;
			}
			// Like Unity, UI elements go in a Canvas (created if needed).
			bool in_canvas = p_id == CREATE_UI_CANVAS;
			for (Node *node = parent; node && !in_canvas; node = node->get_parent()) {
				in_canvas = Object::cast_to<CanvasLayer>(node) || Object::cast_to<Control>(node);
				if (node == edited_scene) {
					break; // Don't look into the editor's own UI.
				}
			}
			if (!in_canvas && edited_scene) {
				CanvasLayer *canvas = memnew(CanvasLayer);
				canvas->set_name("Canvas");
				canvas->add_child(element);
				object = canvas;
				parent = edited_scene;
			} else {
				object = element;
			}
		} break;
		case CREATE_OTHER: {
			SceneTreeDock::get_singleton()->open_add_child_dialog();
			return;
		}
		default:
			return;
	}

	GameObjectUtils::add_object(parent, object, TTR("Create Game Object"));
}

void GameObjectCreateMenu::_notification(int p_what) {
	if (p_what != NOTIFICATION_THEME_CHANGED) {
		return;
	}
	clear(false);
	object_2d_menu->clear(false);
	ui_menu->clear(false);

	add_icon_item(get_editor_theme_icon(SNAME("Node2D")), TTR("Create Empty"), CREATE_EMPTY);
	add_icon_item(get_editor_theme_icon(SNAME("Node2D")), TTR("Create Empty Child"), CREATE_EMPTY_CHILD);
	add_separator();

	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("Sprite2D")), TTR("Square"), CREATE_SQUARE);
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("Sprite2D")), TTR("Circle"), CREATE_CIRCLE);
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("Sprite2D")), TTR("Sprite"), CREATE_SPRITE);
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("AnimatedSprite2D")), TTR("Animated Sprite"), CREATE_ANIMATED_SPRITE);
	object_2d_menu->add_separator();
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("TileMapLayer")), TTR("Tilemap"), CREATE_TILEMAP);
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("Line2D")), TTR("Line"), CREATE_LINE);
	object_2d_menu->add_icon_item(get_editor_theme_icon(SNAME("Polygon2D")), TTR("Polygon"), CREATE_POLYGON);
	add_submenu_node_item(TTR("2D Object"), object_2d_menu);
	set_item_icon(-1, get_editor_theme_icon(SNAME("Sprite2D")));

	add_icon_item(get_editor_theme_icon(SNAME("Camera2D")), TTR("Camera"), CREATE_CAMERA);
	add_icon_item(get_editor_theme_icon(SNAME("AudioStreamPlayer2D")), TTR("Audio Source"), CREATE_AUDIO_SOURCE);
	add_icon_item(get_editor_theme_icon(SNAME("PointLight2D")), TTR("Light 2D"), CREATE_LIGHT);
	add_icon_item(get_editor_theme_icon(SNAME("CPUParticles2D")), TTR("Particle System"), CREATE_PARTICLES);

	ui_menu->add_icon_item(get_editor_theme_icon(SNAME("CanvasLayer")), TTR("Canvas"), CREATE_UI_CANVAS);
	ui_menu->add_icon_item(get_editor_theme_icon(SNAME("Label")), TTR("Text"), CREATE_UI_TEXT);
	ui_menu->add_icon_item(get_editor_theme_icon(SNAME("Button")), TTR("Button"), CREATE_UI_BUTTON);
	ui_menu->add_icon_item(get_editor_theme_icon(SNAME("TextureRect")), TTR("Image"), CREATE_UI_IMAGE);
	ui_menu->add_icon_item(get_editor_theme_icon(SNAME("Panel")), TTR("Panel"), CREATE_UI_PANEL);
	add_submenu_node_item(TTR("UI"), ui_menu);
	set_item_icon(-1, get_editor_theme_icon(SNAME("Control")));

	add_separator();
	add_icon_item(get_editor_theme_icon(SNAME("Add")), TTR("Other Node..."), CREATE_OTHER);
}

GameObjectCreateMenu::GameObjectCreateMenu() {
	object_2d_menu = memnew(PopupMenu);
	add_child(object_2d_menu);
	object_2d_menu->connect(SceneStringName(id_pressed), callable_mp(this, &GameObjectCreateMenu::_id_pressed));
	ui_menu = memnew(PopupMenu);
	add_child(ui_menu);
	ui_menu->connect(SceneStringName(id_pressed), callable_mp(this, &GameObjectCreateMenu::_id_pressed));
	connect(SceneStringName(id_pressed), callable_mp(this, &GameObjectCreateMenu::_id_pressed));
}

/////////////////////////////////////////////////////////////////////////////
// AddComponentDialog

struct ComponentEntry {
	const char *category;
	const char *label;
	const char *id; // kind:argument
	const char *icon;
};

static const ComponentEntry component_entries[] = {
	{ "Rendering", "Sprite Renderer", "component:Sprite2D:SpriteRenderer", "Sprite2D" },
	{ "Rendering", "Animated Sprite", "component:AnimatedSprite2D:AnimatedSprite", "AnimatedSprite2D" },
	{ "Rendering", "Line Renderer", "component:Line2D:LineRenderer", "Line2D" },
	{ "Rendering", "Polygon", "component:Polygon2D:Polygon", "Polygon2D" },
	{ "Rendering", "Particle System", "component:CPUParticles2D:ParticleSystem", "CPUParticles2D" },
	{ "Rendering", "Light 2D", "component:PointLight2D:Light2D", "PointLight2D" },
	{ "Rendering", "Sorting Group", "component:SortingGroup:SortingGroup", "Node" },
	{ "Physics 2D", "Rigidbody 2D", "body:RigidBody2D", "RigidBody2D" },
	{ "Physics 2D", "Kinematic Body 2D", "body:CharacterBody2D", "CharacterBody2D" },
	{ "Physics 2D", "Static Body 2D", "body:StaticBody2D", "StaticBody2D" },
	{ "Physics 2D", "Trigger 2D", "body:Area2D", "Area2D" },
	{ "Physics 2D", "Box Collider 2D", "collider:box", "CollisionShape2D" },
	{ "Physics 2D", "Circle Collider 2D", "collider:circle", "CollisionShape2D" },
	{ "Physics 2D", "Capsule Collider 2D", "collider:capsule", "CollisionShape2D" },
	{ "Physics 2D", "Polygon Collider 2D", "collider:polygon", "CollisionPolygon2D" },
	{ "Audio", "Audio Source", "component:AudioStreamPlayer2D:AudioSource", "AudioStreamPlayer2D" },
	{ "Audio", "Audio Listener", "component:AudioListener2D:AudioListener", "AudioListener2D" },
	{ "Animation", "Animator", "component:AnimationPlayer:Animator", "AnimationPlayer" },
	{ "Animation", "Animation Tree", "component:AnimationTree:AnimationTree", "AnimationTree" },
	{ "Navigation", "Nav Agent 2D", "component:NavigationAgent2D:NavAgent", "NavigationAgent2D" },
	{ "Navigation", "Nav Obstacle 2D", "component:NavigationObstacle2D:NavObstacle", "NavigationObstacle2D" },
	{ "Camera", "Camera", "component:Camera2D:Camera", "Camera2D" },
	{ "Miscellaneous", "Timer", "component:Timer:Timer", "Timer" },
	{ "Miscellaneous", "Remote Transform", "component:RemoteTransform2D:RemoteTransform", "RemoteTransform2D" },
	{ "Miscellaneous", "Visible On Screen", "component:VisibleOnScreenNotifier2D:VisibleOnScreen", "VisibleOnScreenNotifier2D" },
};

void AddComponentDialog::popup_for(Node *p_game_object) {
	target_id = p_game_object ? p_game_object->get_instance_id() : ObjectID();
	search->clear();
	_update_list();
	popup_centered(Size2(380, 520) * EDSCALE);
	search->grab_focus();
}

void AddComponentDialog::_update_list() {
	tree->clear();
	TreeItem *root = tree->create_item();
	const String filter = search->get_text().strip_edges().to_lower();
	const bool searching = !filter.is_empty();
	TreeItem *first_match = nullptr;
	HashMap<String, TreeItem *> categories;

	auto category_item = [&](const String &p_name) -> TreeItem * {
		if (!categories.has(p_name)) {
			TreeItem *item = tree->create_item(root);
			item->set_text(0, p_name);
			item->set_icon(0, get_editor_theme_icon(SNAME("Folder")));
			item->set_collapsed(!searching);
			item->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
			categories[p_name] = item;
		}
		return categories[p_name];
	};
	auto add_entry = [&](const String &p_category, const String &p_label, const String &p_id, const Ref<Texture2D> &p_icon) {
		if (searching && !p_label.to_lower().contains(filter) && !p_id.to_lower().contains(filter)) {
			return;
		}
		TreeItem *item = tree->create_item(category_item(p_category));
		item->set_text(0, p_label);
		item->set_icon(0, p_icon);
		item->set_metadata(0, p_id);
		if (!first_match) {
			first_match = item;
		}
	};

	// Scripts: MonoBehaviours with a class name, and a new script.
	LocalVector<StringName> classes;
	ScriptServer::get_global_class_list(classes);
	classes.sort_custom<StringName::AlphCompare>();
	for (const StringName &name : classes) {
		if (ScriptServer::get_global_class_native_base(name) == SNAME("MonoBehaviour")) {
			add_entry(TTR("Scripts"), name, "script:" + ScriptServer::get_global_class_path(name), get_editor_theme_icon(SNAME("Script")));
		}
	}
	add_entry(TTR("Scripts"), TTR("New Script..."), "new_script:", get_editor_theme_icon(SNAME("ScriptCreate")));

	for (const ComponentEntry &entry : component_entries) {
		const String id = entry.id;
		if (id.begins_with("component:") && !ClassDB::class_exists(id.get_slicec(':', 1))) {
			continue;
		}
		Ref<Texture2D> icon = has_theme_icon(entry.icon, EditorStringName(EditorIcons)) ? get_editor_theme_icon(entry.icon) : get_editor_theme_icon(SNAME("Node"));
		add_entry(TTR(entry.category), TTR(entry.label), id, icon);
	}

	add_entry(TTR("Other"), TTR("Other Node Type..."), "other:", get_editor_theme_icon(SNAME("Add")));

	if (first_match && searching) {
		first_match->select(0);
		tree->scroll_to_item(first_match);
	}
}

void AddComponentDialog::_search_changed(const String &p_text) {
	_update_list();
}

void AddComponentDialog::_search_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && (key->get_keycode() == Key::UP || key->get_keycode() == Key::DOWN || key->get_keycode() == Key::PAGEUP || key->get_keycode() == Key::PAGEDOWN)) {
		tree->gui_input(key); // Navigate the list while typing.
		search->accept_event();
	}
}

void AddComponentDialog::_item_activated() {
	ok_pressed();
}

void AddComponentDialog::_item_selected() {
	// Clicking a category opens or closes it.
	TreeItem *selected = tree->get_selected();
	if (selected && selected->get_metadata(0).get_type() != Variant::STRING) {
		selected->set_collapsed(!selected->is_collapsed());
	}
}

void AddComponentDialog::ok_pressed() {
	TreeItem *selected = tree->get_selected();
	if (!selected || selected->get_metadata(0).get_type() != Variant::STRING) {
		return;
	}
	const String id = selected->get_metadata(0);
	hide();
	_apply(id);
}

void AddComponentDialog::_show_error(const String &p_text) {
	error_dialog->set_text(p_text);
	error_dialog->popup_centered();
}

static Vector2 _sprite_size(Node *p_object) {
	for (int i = 0; i < p_object->get_child_count(false); i++) {
		Sprite2D *sprite = Object::cast_to<Sprite2D>(p_object->get_child(i, false));
		if (sprite && sprite->get_texture().is_valid()) {
			return sprite->get_texture()->get_size() * sprite->get_scale().abs();
		}
	}
	return Vector2(32, 32);
}

void AddComponentDialog::_apply(const String &p_id) {
	Node *target = ObjectDB::get_instance<Node>(target_id);
	if (!target) {
		return;
	}
	SceneTreeDock *dock = SceneTreeDock::get_singleton();
	const int colon = p_id.find_char(':');
	const String kind = p_id.substr(0, colon);
	const String argument = p_id.substr(colon + 1);

	const bool is_plain_object = target->get_class() == "Node2D";
	CollisionObject2D *collision_object = Object::cast_to<CollisionObject2D>(target);

	// Physics: the body is the game object's type (Unity adds a Rigidbody2D component instead).
	auto ensure_body = [&](const String &p_body_class) -> Node * {
		if (target->get_class() == p_body_class) {
			return target;
		}
		if (!is_plain_object && !collision_object) {
			_show_error(vformat(TTR("\"%s\" is a %s, which can't become a physics body.\nUse a game object (Create Empty) with a Sprite Renderer component instead."), target->get_name(), target->get_class()));
			return nullptr;
		}
		Node *body = Object::cast_to<Node>(ClassDB::instantiate(p_body_class));
		ERR_FAIL_NULL_V(body, nullptr);
		dock->replace_node(target, body);
		target_id = body->get_instance_id();
		return body;
	};

	if (kind == "component") {
		const String class_name = argument.get_slicec(':', 0);
		Node *component = Object::cast_to<Node>(ClassDB::instantiate(class_name));
		ERR_FAIL_NULL(component);
		component->set_name(argument.get_slicec(':', 1));
		component->set_meta(SNAME("_component"), true);
		GameObjectUtils::add_object(target, component, TTR("Add Component"));
	} else if (kind == "body") {
		ensure_body(argument);
	} else if (kind == "collider") {
		Node *body = collision_object ? target : ensure_body("StaticBody2D"); // A collider alone is static, like in Unity.
		if (!body) {
			return;
		}
		const Vector2 sprite_size = _sprite_size(body);
		Node *collider = nullptr;
		if (argument == "polygon") {
			CollisionPolygon2D *polygon = memnew(CollisionPolygon2D);
			const Vector2 half = sprite_size / 2;
			polygon->set_polygon({ Vector2(-half.x, -half.y), Vector2(half.x, -half.y), Vector2(half.x, half.y), Vector2(-half.x, half.y) });
			polygon->set_name("PolygonCollider2D");
			collider = polygon;
		} else {
			CollisionShape2D *shape_node = memnew(CollisionShape2D);
			if (argument == "circle") {
				Ref<CircleShape2D> shape;
				shape.instantiate();
				shape->set_radius(MAX(sprite_size.x, sprite_size.y) / 2);
				shape_node->set_shape(shape);
				shape_node->set_name("CircleCollider2D");
			} else if (argument == "capsule") {
				Ref<CapsuleShape2D> shape;
				shape.instantiate();
				shape->set_radius(sprite_size.x / 2);
				shape->set_height(MAX(sprite_size.y, sprite_size.x));
				shape_node->set_shape(shape);
				shape_node->set_name("CapsuleCollider2D");
			} else {
				Ref<RectangleShape2D> shape;
				shape.instantiate();
				shape->set_size(sprite_size);
				shape_node->set_shape(shape);
				shape_node->set_name("BoxCollider2D");
			}
			collider = shape_node;
		}
		GameObjectUtils::add_object(body, collider, TTR("Add Collider"));
	} else if (kind == "script") {
		const Ref<Script> script = ResourceLoader::load(argument);
		Node *component = Object::cast_to<Node>(ClassDB::instantiate("MonoBehaviour"));
		ERR_FAIL_NULL(component);
		if (script.is_valid()) {
			component->set_script(script);
			component->set_name(script->get_global_name());
		}
		GameObjectUtils::add_object(target, component, TTR("Add Component"));
	} else if (kind == "new_script") {
		Node *component = Object::cast_to<Node>(ClassDB::instantiate("MonoBehaviour"));
		ERR_FAIL_NULL(component);
		component->set_name("NewBehaviourScript");
		GameObjectUtils::add_object(target, component, TTR("Add Component"));
		if (component->is_inside_tree()) {
			dock->attach_script_to_component(component);
		}
	} else if (kind == "other") {
		dock->open_add_component_dialog();
	}
}

void AddComponentDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED && is_visible()) {
		_update_list();
	}
}

AddComponentDialog::AddComponentDialog() {
	set_title(TTR("Add Component"));
	set_ok_button_text(TTR("Add"));

	VBoxContainer *vb = memnew(VBoxContainer);
	add_child(vb);

	search = memnew(LineEdit);
	search->set_placeholder(TTR("Search"));
	search->set_clear_button_enabled(true);
	search->connect(SceneStringName(text_changed), callable_mp(this, &AddComponentDialog::_search_changed));
	search->connect(SceneStringName(gui_input), callable_mp(this, &AddComponentDialog::_search_gui_input));
	vb->add_child(search);
	register_text_enter(search);

	tree = memnew(Tree);
	tree->set_hide_root(true);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->connect("item_activated", callable_mp(this, &AddComponentDialog::_item_activated));
	tree->connect("item_selected", callable_mp(this, &AddComponentDialog::_item_selected));
	vb->add_child(tree);

	error_dialog = memnew(AcceptDialog);
	error_dialog->set_title(TTR("Add Component"));
	add_child(error_dialog);
}
