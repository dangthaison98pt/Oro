/**************************************************************************/
/*  unity_singletons.cpp                                                  */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "unity_singletons.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"

// Debug

void Debug::Log(const Variant &p_message) {
	print_line(p_message.stringify());
}

void Debug::LogWarning(const Variant &p_message) {
	WARN_PRINT(p_message.stringify());
}

void Debug::LogError(const Variant &p_message) {
	ERR_PRINT(p_message.stringify());
}

void Debug::Assert(bool p_condition, const Variant &p_message) {
	if (!p_condition) {
		ERR_PRINT(p_message.get_type() == Variant::NIL ? String("Assertion failed") : "Assertion failed: " + p_message.stringify());
	}
}

void Debug::_bind_methods() {
	ClassDB::bind_method(D_METHOD("Log", "message"), &Debug::Log);
	ClassDB::bind_method(D_METHOD("LogWarning", "message"), &Debug::LogWarning);
	ClassDB::bind_method(D_METHOD("LogError", "message"), &Debug::LogError);
	ClassDB::bind_method(D_METHOD("Assert", "condition", "message"), &Debug::Assert, DEFVAL(Variant()));
}

// Mathf

static bool _both_ints(const Variant &p_a, const Variant &p_b) {
	return p_a.get_type() == Variant::INT && p_b.get_type() == Variant::INT;
}

double Mathf::get_pi() const { return Math::PI; }
double Mathf::get_deg2rad() const { return Math::PI / 180.0; }
double Mathf::get_rad2deg() const { return 180.0 / Math::PI; }
double Mathf::get_infinity() const { return Math::INF; }
double Mathf::get_negative_infinity() const { return -Math::INF; }
double Mathf::get_epsilon() const { return 1.401298e-45; }

Variant Mathf::Abs(const Variant &p_value) {
	if (p_value.get_type() == Variant::INT) {
		return Math::abs((int64_t)p_value);
	}
	return Math::abs((double)p_value);
}

Variant Mathf::Min(const Variant &p_a, const Variant &p_b) {
	if (_both_ints(p_a, p_b)) {
		return MIN((int64_t)p_a, (int64_t)p_b);
	}
	return MIN((double)p_a, (double)p_b);
}

Variant Mathf::Max(const Variant &p_a, const Variant &p_b) {
	if (_both_ints(p_a, p_b)) {
		return MAX((int64_t)p_a, (int64_t)p_b);
	}
	return MAX((double)p_a, (double)p_b);
}

Variant Mathf::Clamp(const Variant &p_value, const Variant &p_min, const Variant &p_max) {
	if (_both_ints(p_value, p_min) && p_max.get_type() == Variant::INT) {
		return CLAMP((int64_t)p_value, (int64_t)p_min, (int64_t)p_max);
	}
	return CLAMP((double)p_value, (double)p_min, (double)p_max);
}

double Mathf::Clamp01(double p_value) { return CLAMP(p_value, 0.0, 1.0); }
double Mathf::Lerp(double p_a, double p_b, double p_t) { return Math::lerp(p_a, p_b, CLAMP(p_t, 0.0, 1.0)); }
double Mathf::LerpUnclamped(double p_a, double p_b, double p_t) { return Math::lerp(p_a, p_b, p_t); }

double Mathf::InverseLerp(double p_a, double p_b, double p_value) {
	if (p_a == p_b) {
		return 0.0;
	}
	return CLAMP((p_value - p_a) / (p_b - p_a), 0.0, 1.0);
}

double Mathf::LerpAngle(double p_a, double p_b, double p_t) {
	return p_a + DeltaAngle(p_a, p_b) * CLAMP(p_t, 0.0, 1.0);
}

double Mathf::MoveTowards(double p_current, double p_target, double p_max_delta) {
	if (Math::abs(p_target - p_current) <= p_max_delta) {
		return p_target;
	}
	return p_current + SIGN(p_target - p_current) * p_max_delta;
}

double Mathf::SmoothStep(double p_from, double p_to, double p_t) {
	double t = CLAMP(p_t, 0.0, 1.0);
	t = -2.0 * t * t * t + 3.0 * t * t;
	return p_to * t + p_from * (1.0 - t);
}

double Mathf::Sin(double p_f) { return Math::sin(p_f); }
double Mathf::Cos(double p_f) { return Math::cos(p_f); }
double Mathf::Tan(double p_f) { return Math::tan(p_f); }
double Mathf::Asin(double p_f) { return Math::asin(p_f); }
double Mathf::Acos(double p_f) { return Math::acos(p_f); }
double Mathf::Atan(double p_f) { return Math::atan(p_f); }
double Mathf::Atan2(double p_y, double p_x) { return Math::atan2(p_y, p_x); }
double Mathf::Sqrt(double p_f) { return Math::sqrt(p_f); }
double Mathf::Pow(double p_f, double p_p) { return Math::pow(p_f, p_p); }
double Mathf::Exp(double p_f) { return Math::exp(p_f); }
double Mathf::Log(double p_f) { return Math::log(p_f); }
double Mathf::Log10(double p_f) { return Math::log(p_f) / Math::log(10.0); }
// Unity rounds halves to the nearest even number.
double Mathf::Round(double p_f) { return std::nearbyint(p_f); }
double Mathf::Floor(double p_f) { return Math::floor(p_f); }
double Mathf::Ceil(double p_f) { return Math::ceil(p_f); }
int Mathf::RoundToInt(double p_f) { return (int)std::nearbyint(p_f); }
int Mathf::FloorToInt(double p_f) { return (int)Math::floor(p_f); }
int Mathf::CeilToInt(double p_f) { return (int)Math::ceil(p_f); }
// Unity's Sign returns 1 for zero.
double Mathf::Sign(double p_f) { return p_f >= 0.0 ? 1.0 : -1.0; }

bool Mathf::Approximately(double p_a, double p_b) {
	return Math::abs(p_b - p_a) < MAX(1e-6 * MAX(Math::abs(p_a), Math::abs(p_b)), 1.1754943e-38 * 8.0);
}

double Mathf::Repeat(double p_t, double p_length) {
	return CLAMP(p_t - Math::floor(p_t / p_length) * p_length, 0.0, p_length);
}

double Mathf::PingPong(double p_t, double p_length) {
	const double t = Repeat(p_t, p_length * 2.0);
	return p_length - Math::abs(t - p_length);
}

double Mathf::DeltaAngle(double p_current, double p_target) {
	double delta = Repeat(p_target - p_current, 360.0);
	if (delta > 180.0) {
		delta -= 360.0;
	}
	return delta;
}

void Mathf::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_pi"), &Mathf::get_pi);
	ClassDB::bind_method(D_METHOD("get_deg2rad"), &Mathf::get_deg2rad);
	ClassDB::bind_method(D_METHOD("get_rad2deg"), &Mathf::get_rad2deg);
	ClassDB::bind_method(D_METHOD("get_infinity"), &Mathf::get_infinity);
	ClassDB::bind_method(D_METHOD("get_negative_infinity"), &Mathf::get_negative_infinity);
	ClassDB::bind_method(D_METHOD("get_epsilon"), &Mathf::get_epsilon);

	ClassDB::bind_method(D_METHOD("Abs", "value"), &Mathf::Abs);
	ClassDB::bind_method(D_METHOD("Min", "a", "b"), &Mathf::Min);
	ClassDB::bind_method(D_METHOD("Max", "a", "b"), &Mathf::Max);
	ClassDB::bind_method(D_METHOD("Clamp", "value", "min", "max"), &Mathf::Clamp);
	ClassDB::bind_method(D_METHOD("Clamp01", "value"), &Mathf::Clamp01);
	ClassDB::bind_method(D_METHOD("Lerp", "a", "b", "t"), &Mathf::Lerp);
	ClassDB::bind_method(D_METHOD("LerpUnclamped", "a", "b", "t"), &Mathf::LerpUnclamped);
	ClassDB::bind_method(D_METHOD("InverseLerp", "a", "b", "value"), &Mathf::InverseLerp);
	ClassDB::bind_method(D_METHOD("LerpAngle", "a", "b", "t"), &Mathf::LerpAngle);
	ClassDB::bind_method(D_METHOD("MoveTowards", "current", "target", "max_delta"), &Mathf::MoveTowards);
	ClassDB::bind_method(D_METHOD("SmoothStep", "from", "to", "t"), &Mathf::SmoothStep);
	ClassDB::bind_method(D_METHOD("Sin", "f"), &Mathf::Sin);
	ClassDB::bind_method(D_METHOD("Cos", "f"), &Mathf::Cos);
	ClassDB::bind_method(D_METHOD("Tan", "f"), &Mathf::Tan);
	ClassDB::bind_method(D_METHOD("Asin", "f"), &Mathf::Asin);
	ClassDB::bind_method(D_METHOD("Acos", "f"), &Mathf::Acos);
	ClassDB::bind_method(D_METHOD("Atan", "f"), &Mathf::Atan);
	ClassDB::bind_method(D_METHOD("Atan2", "y", "x"), &Mathf::Atan2);
	ClassDB::bind_method(D_METHOD("Sqrt", "f"), &Mathf::Sqrt);
	ClassDB::bind_method(D_METHOD("Pow", "f", "p"), &Mathf::Pow);
	ClassDB::bind_method(D_METHOD("Exp", "f"), &Mathf::Exp);
	ClassDB::bind_method(D_METHOD("Log", "f"), &Mathf::Log);
	ClassDB::bind_method(D_METHOD("Log10", "f"), &Mathf::Log10);
	ClassDB::bind_method(D_METHOD("Round", "f"), &Mathf::Round);
	ClassDB::bind_method(D_METHOD("Floor", "f"), &Mathf::Floor);
	ClassDB::bind_method(D_METHOD("Ceil", "f"), &Mathf::Ceil);
	ClassDB::bind_method(D_METHOD("RoundToInt", "f"), &Mathf::RoundToInt);
	ClassDB::bind_method(D_METHOD("FloorToInt", "f"), &Mathf::FloorToInt);
	ClassDB::bind_method(D_METHOD("CeilToInt", "f"), &Mathf::CeilToInt);
	ClassDB::bind_method(D_METHOD("Sign", "f"), &Mathf::Sign);
	ClassDB::bind_method(D_METHOD("Approximately", "a", "b"), &Mathf::Approximately);
	ClassDB::bind_method(D_METHOD("Repeat", "t", "length"), &Mathf::Repeat);
	ClassDB::bind_method(D_METHOD("PingPong", "t", "length"), &Mathf::PingPong);
	ClassDB::bind_method(D_METHOD("DeltaAngle", "current", "target"), &Mathf::DeltaAngle);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "PI", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_pi");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Deg2Rad", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_deg2rad");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Rad2Deg", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_rad2deg");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Infinity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_infinity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "NegativeInfinity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_negative_infinity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Epsilon", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_epsilon");
}

// Random

Variant Random::Range(const Variant &p_min, const Variant &p_max) {
	if (_both_ints(p_min, p_max)) {
		// Like Unity, the integer version excludes max.
		const int64_t from = p_min;
		const int64_t to = p_max;
		if (to <= from) {
			return from;
		}
		return Math::random((int)from, (int)(to - 1));
	}
	return Math::random((double)p_min, (double)p_max);
}

double Random::get_value() const {
	return Math::randf();
}

Vector2 Random::get_inside_unit_circle() const {
	const double angle = Math::randf() * Math::TAU;
	const double radius = Math::sqrt((double)Math::randf());
	return Vector2(Math::cos(angle), Math::sin(angle)) * radius;
}

Vector2 Random::get_on_unit_circle() const {
	const double angle = Math::randf() * Math::TAU;
	return Vector2(Math::cos(angle), Math::sin(angle));
}

double Random::get_rotation() const {
	return Math::randf() * 360.0;
}

void Random::InitState(int64_t p_seed) {
	Math::seed((uint64_t)p_seed);
}

void Random::_bind_methods() {
	ClassDB::bind_method(D_METHOD("Range", "min", "max"), &Random::Range);
	ClassDB::bind_method(D_METHOD("InitState", "seed"), &Random::InitState);
	ClassDB::bind_method(D_METHOD("get_value"), &Random::get_value);
	ClassDB::bind_method(D_METHOD("get_inside_unit_circle"), &Random::get_inside_unit_circle);
	ClassDB::bind_method(D_METHOD("get_on_unit_circle"), &Random::get_on_unit_circle);
	ClassDB::bind_method(D_METHOD("get_rotation"), &Random::get_rotation);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_value");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "insideUnitCircle", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_inside_unit_circle");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "onUnitCircle", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_on_unit_circle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_rotation");
}

// SceneManager

String SceneManager::_find_scene(const String &p_dir, const String &p_file) {
	Ref<DirAccess> dir = DirAccess::open(p_dir);
	if (dir.is_null()) {
		return String();
	}
	dir->list_dir_begin();
	String found;
	for (String name = dir->get_next(); !name.is_empty() && found.is_empty(); name = dir->get_next()) {
		if (name.begins_with(".") || name == "addons") {
			continue;
		}
		const String path = p_dir.path_join(name);
		if (dir->current_is_dir()) {
			found = _find_scene(path, p_file);
		} else if (name.to_lower() == p_file.to_lower()) {
			found = path;
		}
	}
	dir->list_dir_end();
	return found;
}

void SceneManager::LoadScene(const String &p_scene) {
	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL(tree);

	String path = p_scene;
	if (!path.begins_with("res://") && !path.begins_with("uid://")) {
		// Unity loads scenes by name: "Level1" -> res://.../Level1.tscn
		String file = path.get_file();
		if (file.get_extension().is_empty()) {
			file += ".tscn";
		}
		path = _find_scene("res://", file);
		if (path.is_empty() && file.ends_with(".tscn")) {
			path = _find_scene("res://", file.get_basename() + ".scn");
		}
		ERR_FAIL_COND_MSG(path.is_empty(), vformat("Scene \"%s\" was not found in the project.", p_scene));
	}
	tree->call_deferred(SNAME("change_scene_to_file"), path);
}

void SceneManager::ReloadScene() {
	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL(tree);
	tree->call_deferred(SNAME("reload_current_scene"));
}

Dictionary SceneManager::GetActiveScene() {
	Dictionary scene;
	SceneTree *tree = SceneTree::get_singleton();
	Node *current = tree ? tree->get_current_scene() : nullptr;
	const String path = current ? current->get_scene_file_path() : String();
	scene["name"] = path.is_empty() ? (current ? String(current->get_name()) : String()) : path.get_file().get_basename();
	scene["path"] = path;
	scene["rootCount"] = current ? 1 : 0;
	return scene;
}

void SceneManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("LoadScene", "scene"), &SceneManager::LoadScene);
	ClassDB::bind_method(D_METHOD("ReloadScene"), &SceneManager::ReloadScene);
	ClassDB::bind_method(D_METHOD("GetActiveScene"), &SceneManager::GetActiveScene);
}

// Application

void Application::Quit(int p_exit_code) {
	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL(tree);
	tree->quit(p_exit_code);
}

void Application::OpenURL(const String &p_url) {
	OS::get_singleton()->shell_open(p_url);
}

int Application::get_target_frame_rate() const {
	const int fps = Engine::get_singleton()->get_max_fps();
	return fps == 0 ? -1 : fps;
}

void Application::set_target_frame_rate(int p_fps) {
	Engine::get_singleton()->set_max_fps(MAX(p_fps, 0));
}

bool Application::is_editor() const {
	return OS::get_singleton()->has_feature("editor");
}

String Application::get_platform() const {
	return OS::get_singleton()->get_name();
}

String Application::get_product_name() const {
	return GLOBAL_GET("application/config/name");
}

String Application::get_version() const {
	return GLOBAL_GET("application/config/version");
}

String Application::get_data_path() const {
	return "res://";
}

String Application::get_persistent_data_path() const {
	return OS::get_singleton()->get_user_data_dir();
}

void Application::_bind_methods() {
	ClassDB::bind_method(D_METHOD("Quit", "exit_code"), &Application::Quit, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("OpenURL", "url"), &Application::OpenURL);
	ClassDB::bind_method(D_METHOD("get_target_frame_rate"), &Application::get_target_frame_rate);
	ClassDB::bind_method(D_METHOD("set_target_frame_rate", "fps"), &Application::set_target_frame_rate);
	ClassDB::bind_method(D_METHOD("is_editor"), &Application::is_editor);
	ClassDB::bind_method(D_METHOD("get_platform"), &Application::get_platform);
	ClassDB::bind_method(D_METHOD("get_product_name"), &Application::get_product_name);
	ClassDB::bind_method(D_METHOD("get_version"), &Application::get_version);
	ClassDB::bind_method(D_METHOD("get_data_path"), &Application::get_data_path);
	ClassDB::bind_method(D_METHOD("get_persistent_data_path"), &Application::get_persistent_data_path);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "targetFrameRate", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_target_frame_rate", "get_target_frame_rate");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "isEditor", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "is_editor");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "platform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_platform");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "productName", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_product_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "version", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_version");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dataPath", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_data_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "persistentDataPath", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_persistent_data_path");
}

// KeyCode

static void _bind_key(const char *p_name, Key p_key) {
	ClassDB::bind_integer_constant(KeyCode::get_class_static(), StringName(), p_name, (int64_t)p_key);
}

void KeyCode::_bind_methods() {
	_bind_key("None", Key::NONE);
	char letter[2] = { 0, 0 };
	for (int i = 0; i < 26; i++) {
		letter[0] = 'A' + i;
		_bind_key(letter, Key((int)Key::A + i));
	}
	static const char *alpha[10] = { "Alpha0", "Alpha1", "Alpha2", "Alpha3", "Alpha4", "Alpha5", "Alpha6", "Alpha7", "Alpha8", "Alpha9" };
	static const char *keypad[10] = { "Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4", "Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9" };
	for (int i = 0; i < 10; i++) {
		_bind_key(alpha[i], Key((int)Key::KEY_0 + i));
		_bind_key(keypad[i], Key((int)Key::KP_0 + i));
	}
	static const char *function[12] = { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };
	for (int i = 0; i < 12; i++) {
		_bind_key(function[i], Key((int)Key::F1 + i));
	}
	_bind_key("Space", Key::SPACE);
	_bind_key("Return", Key::ENTER);
	_bind_key("KeypadEnter", Key::KP_ENTER);
	_bind_key("Escape", Key::ESCAPE);
	_bind_key("Tab", Key::TAB);
	_bind_key("Backspace", Key::BACKSPACE);
	_bind_key("Delete", Key::KEY_DELETE);
	_bind_key("Insert", Key::INSERT);
	_bind_key("Home", Key::HOME);
	_bind_key("End", Key::END);
	_bind_key("PageUp", Key::PAGEUP);
	_bind_key("PageDown", Key::PAGEDOWN);
	_bind_key("UpArrow", Key::UP);
	_bind_key("DownArrow", Key::DOWN);
	_bind_key("LeftArrow", Key::LEFT);
	_bind_key("RightArrow", Key::RIGHT);
	// The engine doesn't tell left and right modifiers apart in key codes.
	_bind_key("LeftShift", Key::SHIFT);
	_bind_key("RightShift", Key::SHIFT);
	_bind_key("LeftControl", Key::CTRL);
	_bind_key("RightControl", Key::CTRL);
	_bind_key("LeftAlt", Key::ALT);
	_bind_key("RightAlt", Key::ALT);
	_bind_key("LeftCommand", Key::META);
	_bind_key("RightCommand", Key::META);
	_bind_key("CapsLock", Key::CAPSLOCK);
	_bind_key("Minus", Key::MINUS);
	_bind_key("Equals", Key::EQUAL);
	_bind_key("Comma", Key::COMMA);
	_bind_key("Period", Key::PERIOD);
	_bind_key("Slash", Key::SLASH);
	_bind_key("Backslash", Key::BACKSLASH);
	_bind_key("Semicolon", Key::SEMICOLON);
	_bind_key("Quote", Key::APOSTROPHE);
	_bind_key("LeftBracket", Key::BRACKETLEFT);
	_bind_key("RightBracket", Key::BRACKETRIGHT);
	_bind_key("BackQuote", Key::QUOTELEFT);
}
