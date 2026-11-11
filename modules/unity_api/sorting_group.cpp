/**************************************************************************/
/*  sorting_group.cpp                                                     */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "sorting_group.h"

#include "core/object/class_db.h"
#include "scene/main/canvas_item.h"

CanvasItem *SortingGroup::_get_target() const {
	return Object::cast_to<CanvasItem>(get_parent());
}

void SortingGroup::_apply() {
	CanvasItem *target = _get_target();
	if (!target) {
		return;
	}
	target->set_modulate(color);
	target->set_z_index(sorting_order);
	target->set_y_sort_enabled(y_sort);
	switch (texture_filter) {
		case TEXTURE_FILTER_NEAREST:
			target->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			break;
		case TEXTURE_FILTER_LINEAR:
			target->set_texture_filter(CanvasItem::TEXTURE_FILTER_LINEAR);
			break;
		default:
			target->set_texture_filter(CanvasItem::TEXTURE_FILTER_PARENT_NODE);
			break;
	}
	applied_to = target->get_instance_id();
}

void SortingGroup::_reset(CanvasItem *p_target) {
	if (!p_target) {
		return;
	}
	p_target->set_modulate(Color(1, 1, 1, 1));
	p_target->set_z_index(0);
	p_target->set_y_sort_enabled(false);
	p_target->set_texture_filter(CanvasItem::TEXTURE_FILTER_PARENT_NODE);
}

void SortingGroup::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			_apply();
			update_configuration_warnings();
		} break;
		case NOTIFICATION_UNPARENTED: {
			// The component was removed: the game object goes back to normal.
			CanvasItem *previous = ObjectDB::get_instance<CanvasItem>(applied_to);
			if (previous && !previous->is_queued_for_deletion()) {
				_reset(previous);
			}
			applied_to = ObjectID();
		} break;
	}
}

void SortingGroup::set_color(const Color &p_color) {
	color = p_color;
	_apply();
}

Color SortingGroup::get_color() const {
	return color;
}

void SortingGroup::set_sorting_order(int p_order) {
	sorting_order = CLAMP(p_order, RSE::CANVAS_ITEM_Z_MIN, RSE::CANVAS_ITEM_Z_MAX);
	_apply();
}

int SortingGroup::get_sorting_order() const {
	return sorting_order;
}

void SortingGroup::set_y_sort(bool p_enabled) {
	y_sort = p_enabled;
	_apply();
}

bool SortingGroup::is_y_sort() const {
	return y_sort;
}

void SortingGroup::set_texture_filter(TextureFilter p_filter) {
	texture_filter = p_filter;
	_apply();
}

SortingGroup::TextureFilter SortingGroup::get_texture_filter() const {
	return texture_filter;
}

PackedStringArray SortingGroup::get_configuration_warnings() const {
	PackedStringArray warnings = Node::get_configuration_warnings();
	if (get_parent() && !_get_target()) {
		warnings.push_back(RTR("A Sorting Group only works on a 2D game object (Node2D, sprite, UI...)."));
	}
	return warnings;
}

void SortingGroup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_color", "color"), &SortingGroup::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &SortingGroup::get_color);
	ClassDB::bind_method(D_METHOD("set_sorting_order", "order"), &SortingGroup::set_sorting_order);
	ClassDB::bind_method(D_METHOD("get_sorting_order"), &SortingGroup::get_sorting_order);
	ClassDB::bind_method(D_METHOD("set_y_sort", "enabled"), &SortingGroup::set_y_sort);
	ClassDB::bind_method(D_METHOD("is_y_sort"), &SortingGroup::is_y_sort);
	ClassDB::bind_method(D_METHOD("set_texture_filter", "filter"), &SortingGroup::set_texture_filter);
	ClassDB::bind_method(D_METHOD("get_texture_filter"), &SortingGroup::get_texture_filter);

	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sorting_order", PROPERTY_HINT_RANGE, itos(RSE::CANVAS_ITEM_Z_MIN) + "," + itos(RSE::CANVAS_ITEM_Z_MAX) + ",1"), "set_sorting_order", "get_sorting_order");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "y_sort"), "set_y_sort", "is_y_sort");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "filter_mode", PROPERTY_HINT_ENUM, "Inherit,Nearest (Pixel Art),Linear"), "set_texture_filter", "get_texture_filter");

	BIND_ENUM_CONSTANT(TEXTURE_FILTER_INHERIT);
	BIND_ENUM_CONSTANT(TEXTURE_FILTER_NEAREST);
	BIND_ENUM_CONSTANT(TEXTURE_FILTER_LINEAR);
}
