/**************************************************************************/
/*  sorting_group.h                                                       */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "scene/main/node.h"

class CanvasItem;

// Unity-style Sorting Group component: add it to a game object to color, fade and sort the
// object and all its children together (e.g. a character made of several sprites).
// It drives its game object's group settings (modulate, z index, Y sort, texture filter) and
// resets them when it's removed.
class SortingGroup : public Node {
	GDCLASS(SortingGroup, Node);

public:
	enum TextureFilter {
		TEXTURE_FILTER_INHERIT,
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
	};

private:
	Color color = Color(1, 1, 1, 1);
	int sorting_order = 0;
	bool y_sort = false;
	TextureFilter texture_filter = TEXTURE_FILTER_INHERIT;
	ObjectID applied_to;

	CanvasItem *_get_target() const;
	void _apply();
	void _reset(CanvasItem *p_target);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_color(const Color &p_color);
	Color get_color() const;
	void set_sorting_order(int p_order);
	int get_sorting_order() const;
	void set_y_sort(bool p_enabled);
	bool is_y_sort() const;
	void set_texture_filter(TextureFilter p_filter);
	TextureFilter get_texture_filter() const;

	PackedStringArray get_configuration_warnings() const override;
};

VARIANT_ENUM_CAST(SortingGroup::TextureFilter);
