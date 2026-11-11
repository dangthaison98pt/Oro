/**************************************************************************/
/*  oroscript_translator.h                                                */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "core/string/ustring.h"
#include "core/templates/vector.h"

// OroScript (.oro) is Orostride's C#-like scripting language:
//
//     public class Player : MonoBehaviour
//     {
//         public float speed = 5f;
//         void Update() { transform.Translate(Vector2.right * speed * Time.deltaTime); }
//     }
//
// Stage 1: this front end translates OroScript to the engine's script language, line by line
// (each statement keeps its line number, so errors and breakpoints point at the .oro file),
// and the existing analyzer, compiler and virtual machine run the result.
class OroScriptTranslator {
public:
	struct Error {
		int line = 1;
		int column = 1;
		String message;
	};

	static bool is_oroscript_path(const String &p_path) {
		return p_path.get_extension().to_lower() == "oro";
	}

	// p_path picks the script's class: the one named like the file (like Unity), else the first one.
	static String translate(const String &p_source, const String &p_path, Vector<Error> &r_errors);
};
