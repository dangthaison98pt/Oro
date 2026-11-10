/**************************************************************************/
/*  unity_singletons.h                                                    */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#pragma once

#include "core/object/object.h"
#include "core/variant/dictionary.h"

// Unity-style static classes, exposed to scripts as global singletons
// (Debug.Log, Mathf.Clamp, Random.Range, SceneManager.LoadScene, Application.Quit).

class Debug : public Object {
	GDCLASS(Debug, Object);

protected:
	static void _bind_methods();

public:
	void Log(const Variant &p_message);
	void LogWarning(const Variant &p_message);
	void LogError(const Variant &p_message);
	void Assert(bool p_condition, const Variant &p_message = Variant());
};

class Mathf : public Object {
	GDCLASS(Mathf, Object);

protected:
	static void _bind_methods();

public:
	double get_pi() const;
	double get_deg2rad() const;
	double get_rad2deg() const;
	double get_infinity() const;
	double get_negative_infinity() const;
	double get_epsilon() const;

	Variant Abs(const Variant &p_value);
	Variant Min(const Variant &p_a, const Variant &p_b);
	Variant Max(const Variant &p_a, const Variant &p_b);
	Variant Clamp(const Variant &p_value, const Variant &p_min, const Variant &p_max);
	double Clamp01(double p_value);
	double Lerp(double p_a, double p_b, double p_t);
	double LerpUnclamped(double p_a, double p_b, double p_t);
	double InverseLerp(double p_a, double p_b, double p_value);
	double LerpAngle(double p_a, double p_b, double p_t);
	double MoveTowards(double p_current, double p_target, double p_max_delta);
	double SmoothStep(double p_from, double p_to, double p_t);
	double Sin(double p_f);
	double Cos(double p_f);
	double Tan(double p_f);
	double Asin(double p_f);
	double Acos(double p_f);
	double Atan(double p_f);
	double Atan2(double p_y, double p_x);
	double Sqrt(double p_f);
	double Pow(double p_f, double p_p);
	double Exp(double p_f);
	double Log(double p_f);
	double Log10(double p_f);
	double Round(double p_f);
	double Floor(double p_f);
	double Ceil(double p_f);
	int RoundToInt(double p_f);
	int FloorToInt(double p_f);
	int CeilToInt(double p_f);
	double Sign(double p_f);
	bool Approximately(double p_a, double p_b);
	double Repeat(double p_t, double p_length);
	double PingPong(double p_t, double p_length);
	double DeltaAngle(double p_current, double p_target);
};

class Random : public Object {
	GDCLASS(Random, Object);

protected:
	static void _bind_methods();

public:
	Variant Range(const Variant &p_min, const Variant &p_max);
	double get_value() const;
	Vector2 get_inside_unit_circle() const;
	Vector2 get_on_unit_circle() const;
	double get_rotation() const;
	void InitState(int64_t p_seed);
};

class SceneManager : public Object {
	GDCLASS(SceneManager, Object);

	static String _find_scene(const String &p_dir, const String &p_file);

protected:
	static void _bind_methods();

public:
	void LoadScene(const String &p_scene);
	Dictionary GetActiveScene();
	void ReloadScene();
};

class Application : public Object {
	GDCLASS(Application, Object);

protected:
	static void _bind_methods();

public:
	void Quit(int p_exit_code = 0);
	void OpenURL(const String &p_url);
	int get_target_frame_rate() const;
	void set_target_frame_rate(int p_fps);
	bool is_editor() const;
	String get_platform() const;
	String get_product_name() const;
	String get_version() const;
	String get_data_path() const;
	String get_persistent_data_path() const;
};

// Key codes for Input.GetKey(KeyCode.Space). The values are the engine's key codes.
class KeyCode : public Object {
	GDCLASS(KeyCode, Object);

protected:
	static void _bind_methods();
};
