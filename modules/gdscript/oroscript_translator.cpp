/**************************************************************************/
/*  oroscript_translator.cpp                                              */
/**************************************************************************/
/* This file is part of Orostride Engine (based on Godot Engine).         */
/* Copyright (c) 2014-present Godot Engine contributors. MIT license,     */
/* see LICENSE.txt.                                                       */
/**************************************************************************/

#include "oroscript_translator.h"

#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

namespace {

struct Token {
	enum Type {
		IDENT,
		NUMBER,
		STRING, // Already converted to a quoted literal of the target language.
		INTERPOLATED, // $"..." raw content (between the quotes).
		PUNCT,
		END,
	};
	Type type = END;
	String text;
	int line = 1;
	int column = 1;
	int end_column = 1; // Column just after the token (to detect `>>`).
};

class Lexer {
	const String &src;
	int pos = 0;
	int line = 1;
	int column = 1;
	Vector<OroScriptTranslator::Error> &errors;

	char32_t peek(int p_offset = 0) const {
		const int i = pos + p_offset;
		return i < src.length() ? src[i] : 0;
	}

	void advance(int p_count = 1) {
		for (int i = 0; i < p_count && pos < src.length(); i++) {
			if (src[pos] == '\n') {
				line++;
				column = 1;
			} else {
				column++;
			}
			pos++;
		}
	}

	static bool is_ident_start(char32_t c) { return is_unicode_identifier_start(c) || c == '_'; }
	static bool is_ident_char(char32_t c) { return is_unicode_identifier_continue(c) || c == '_'; }

	void error(const String &p_message) {
		OroScriptTranslator::Error e;
		e.line = line;
		e.column = column;
		e.message = p_message;
		errors.push_back(e);
	}

	// Reads a regular "..." string (escapes are the same in both languages).
	String read_string_body() {
		String body;
		while (pos < src.length() && peek() != '"' && peek() != '\n') {
			if (peek() == '\\' && pos + 1 < src.length()) {
				body += peek();
				advance();
			}
			body += peek();
			advance();
		}
		if (peek() != '"') {
			error("Unterminated string.");
		} else {
			advance();
		}
		return body;
	}

	// Reads a verbatim @"..." string and escapes it.
	String read_verbatim_body() {
		String body;
		while (pos < src.length()) {
			const char32_t c = peek();
			if (c == '"') {
				if (peek(1) == '"') {
					body += "\\\"";
					advance(2);
					continue;
				}
				advance();
				return body;
			}
			if (c == '\\') {
				body += "\\\\";
			} else if (c == '\n') {
				body += "\\n";
			} else if (c != '\r') {
				body += c;
			}
			advance();
		}
		error("Unterminated verbatim string.");
		return body;
	}

public:
	Lexer(const String &p_src, Vector<OroScriptTranslator::Error> &r_errors) :
			src(p_src), errors(r_errors) {}

	Vector<Token> tokenize() {
		Vector<Token> tokens;
		static const char *puncts[] = { "??=", "<<=", "?.", "??", "=>", "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", "::", nullptr };

		while (pos < src.length()) {
			const char32_t c = peek();
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0xFEFF) {
				advance();
				continue;
			}
			// Comments and preprocessor lines (#region, ...).
			if (c == '/' && peek(1) == '/') {
				while (pos < src.length() && peek() != '\n') {
					advance();
				}
				continue;
			}
			if (c == '/' && peek(1) == '*') {
				advance(2);
				while (pos < src.length() && !(peek() == '*' && peek(1) == '/')) {
					advance();
				}
				advance(2);
				continue;
			}
			if (c == '#') {
				while (pos < src.length() && peek() != '\n') {
					advance();
				}
				continue;
			}

			Token t;
			t.line = line;
			t.column = column;

			if (is_ident_start(c) || (c == '@' && is_ident_start(peek(1)))) {
				if (c == '@') {
					advance(); // Verbatim identifier.
				}
				t.type = Token::IDENT;
				while (pos < src.length() && is_ident_char(peek())) {
					t.text += peek();
					advance();
				}
			} else if (is_digit(c) || (c == '.' && is_digit(peek(1)))) {
				t.type = Token::NUMBER;
				String number;
				if (c == '.') {
					number = "0";
				}
				bool hex = false;
				if (c == '0' && (peek(1) == 'x' || peek(1) == 'X' || peek(1) == 'b' || peek(1) == 'B')) {
					hex = true;
					number += peek();
					number += peek(1);
					advance(2);
				}
				while (pos < src.length()) {
					const char32_t d = peek();
					if (is_digit(d) || (hex && is_hex_digit(d)) || d == '_') {
						if (d != '_') {
							number += d;
						}
					} else if (!hex && d == '.' && is_digit(peek(1))) {
						number += d;
					} else if (!hex && (d == 'e' || d == 'E') && (is_digit(peek(1)) || ((peek(1) == '-' || peek(1) == '+') && is_digit(peek(2))))) {
						number += d;
						advance();
						number += peek();
					} else {
						break;
					}
					advance();
				}
				// Suffixes: 5f, 2.5d, 1m (real numbers), 10u, 10L (integers).
				bool real = false;
				while (pos < src.length() && is_ascii_alphabet_char(peek())) {
					const char32_t s = peek();
					if (!hex && (s == 'f' || s == 'F' || s == 'd' || s == 'D' || s == 'm' || s == 'M')) {
						real = true;
					}
					advance();
				}
				if (real && !number.contains_char('.') && !number.contains_char('e') && !number.contains_char('E')) {
					number += ".0";
				}
				t.text = number;
			} else if (c == '"' || (c == '@' && peek(1) == '"')) {
				t.type = Token::STRING;
				if (c == '@') {
					advance(2);
					t.text = "\"" + read_verbatim_body() + "\"";
				} else {
					advance();
					t.text = "\"" + read_string_body() + "\"";
				}
			} else if (c == '$' && (peek(1) == '"' || (peek(1) == '@' && peek(2) == '"'))) {
				t.type = Token::INTERPOLATED;
				if (peek(1) == '@') {
					advance(3);
					t.text = read_verbatim_body();
				} else {
					advance(2);
					t.text = read_string_body();
				}
			} else if (c == '\'') {
				t.type = Token::STRING;
				advance();
				String body;
				while (pos < src.length() && peek() != '\'' && peek() != '\n') {
					if (peek() == '\\') {
						body += peek();
						advance();
					}
					if (peek() == '"') {
						body += "\\";
					}
					body += peek();
					advance();
				}
				if (peek() == '\'') {
					advance();
				} else {
					error("Unterminated character literal.");
				}
				t.text = "\"" + body + "\"";
			} else {
				t.type = Token::PUNCT;
				bool matched = false;
				for (int i = 0; puncts[i]; i++) {
					const String p = puncts[i];
					bool ok = true;
					for (int j = 0; j < p.length(); j++) {
						if (peek(j) != p[j]) {
							ok = false;
							break;
						}
					}
					if (ok) {
						t.text = p;
						advance(p.length());
						matched = true;
						break;
					}
				}
				if (!matched) {
					if (String("{}()[];,.:?+-*/%<>=!&|^~").contains_char(c)) {
						t.text = String::chr(c);
						advance();
					} else {
						error(vformat("Unexpected character '%s'.", String::chr(c)));
						advance();
						continue;
					}
				}
			}
			t.end_column = column;
			tokens.push_back(t);
		}

		Token end;
		end.type = Token::END;
		end.line = line;
		end.column = column;
		tokens.push_back(end);
		return tokens;
	}
};

struct OutLine {
	int indent = 0;
	String text;
	bool header = false; // Ends with ':' and opens a block.
	bool inline_body = false; // A header whose body continues on the same line.
	int body_indent = 0;
};

struct TypeInfo {
	String hint; // Type hint in the target language, empty for Variant.
	String name; // Mapped type name, without generics (to call constructors).
	bool is_void = false;
	bool is_var = false;
	bool exportable = false;
};

class Translator {
	Vector<Token> tokens;
	int pos = 0;
	Vector<Vector<OutLine>> out;
	Vector<OroScriptTranslator::Error> &errors;
	HashSet<String> local_types; // Enums and classes declared in this file.
	HashSet<String> string_expressions; // Translated expressions known to be strings.

	String mark_string(const String &p_expression) {
		string_expressions.insert(p_expression);
		return p_expression;
	}
	bool is_string(const String &p_expression) const {
		return string_expressions.has(p_expression) || p_expression.begins_with("\"") || p_expression.begins_with("str(");
	}
	bool has_top_class = false;

	// Errors: fail() records the first error of a statement and makes the parser see the end of
	// the input, so every parsing function returns quickly. parse_statement() and parse_member()
	// then skip to the next statement. (The engine is built without C++ exceptions.)
	bool failed = false;
	int fail_pos = 0;

	// --- Tokens ---

	const Token &cur() const { return failed ? tokens[tokens.size() - 1] : tokens[pos]; }
	const Token &peek_token(int p_offset = 1) const { return tokens[MIN(pos + p_offset, tokens.size() - 1)]; }
	bool at_end() const { return cur().type == Token::END; }
	bool is(const char *p_text) const { return (cur().type == Token::PUNCT || cur().type == Token::IDENT) && cur().text == p_text; }
	bool is_at(int p_offset, const char *p_text) const {
		const Token &t = peek_token(p_offset);
		return (t.type == Token::PUNCT || t.type == Token::IDENT) && t.text == p_text;
	}
	bool is_ident() const { return cur().type == Token::IDENT; }

	void fail(const String &p_message, const Token &p_token) {
		if (failed) {
			return;
		}
		OroScriptTranslator::Error e;
		e.line = p_token.line;
		e.column = p_token.column;
		e.message = p_message;
		errors.push_back(e);
		failed = true;
		fail_pos = pos;
	}
	void fail(const String &p_message) { fail(p_message, cur()); }

	bool accept(const char *p_text) {
		if (is(p_text)) {
			pos++;
			return true;
		}
		return false;
	}

	void expect(const char *p_text) {
		if (accept(p_text)) {
			return;
		}
		if (String(p_text) == ";" && pos > 0 && !failed) {
			// Like C# compilers, report a missing ';' at the end of the previous token.
			Token at = tokens[pos - 1];
			at.column = at.end_column;
			fail("Expected \";\" at the end of the statement.", at);
			return;
		}
		fail(vformat("Expected \"%s\", found \"%s\".", p_text, at_end() ? String("end of file") : cur().text));
	}

	String expect_ident() {
		if (!is_ident()) {
			fail(vformat("Expected a name, found \"%s\".", at_end() ? String("end of file") : cur().text));
			return String();
		}
		return tokens[pos++].text;
	}

	// Skips to the end of the current statement or member after an error.
	void recover() {
		int depth = 0;
		pos = MIN(pos, tokens.size() - 1);
		while (!at_end()) {
			if (is("{")) {
				depth++;
			} else if (is("}")) {
				if (depth == 0) {
					return;
				}
				depth--;
				if (depth == 0) {
					pos++;
					return;
				}
			} else if (is(";") && depth == 0) {
				pos++;
				return;
			}
			pos++;
		}
	}

	// Called after parsing a statement or member that started at p_start.
	void recover_if_failed(int p_start) {
		if (!failed) {
			return;
		}
		failed = false;
		pos = MAX(fail_pos, p_start);
		if (pos == p_start) {
			pos++;
		}
		recover();
	}

	// --- Output ---

	void emit(int p_line, int p_indent, const String &p_text, bool p_header = false) {
		Vector<OutLine> &slot = out.write[CLAMP(p_line, 1, out.size()) - 1];
		OutLine line;
		line.indent = p_indent;
		line.text = p_text;
		line.header = p_header;
		if (!slot.is_empty()) {
			OutLine &last = slot.write[slot.size() - 1];
			if (!p_header) {
				if (last.header && !last.inline_body && p_indent > last.indent) {
					// One-line body: `void F() { Do(); }`
					last.text += " " + p_text;
					last.inline_body = true;
					last.body_indent = p_indent;
					return;
				}
				if ((last.inline_body && p_indent == last.body_indent) || (!last.header && p_indent == last.indent)) {
					last.text += "; " + p_text;
					return;
				}
			}
		}
		slot.push_back(line); // May add a line (only for unusual layouts).
	}

	// --- Types ---

	static String map_type_name(const String &p_name) {
		static HashMap<String, String> aliases;
		if (aliases.is_empty()) {
			const char *pairs[][2] = {
				{ "int", "int" }, { "long", "int" }, { "short", "int" }, { "byte", "int" }, { "sbyte", "int" },
				{ "uint", "int" }, { "ulong", "int" }, { "ushort", "int" },
				{ "float", "float" }, { "double", "float" }, { "decimal", "float" },
				{ "bool", "bool" }, { "string", "String" }, { "char", "String" }, { "object", "Variant" },
				{ "KeyCode", "int" },
				// Unity names for the engine's classes.
				{ "GameObject", "Node" }, { "Component", "Node" }, { "Behaviour", "Node" }, { "MonoBehaviour", "MonoBehaviour" },
				{ "Rigidbody2D", "RigidBody2D" }, { "SpriteRenderer", "Sprite2D" }, { "AudioSource", "AudioStreamPlayer2D" },
				{ "AudioClip", "AudioStream" }, { "Sprite", "Texture2D" }, { "Animator", "AnimationPlayer" }, { "Camera", "Camera2D" },
				{ "Collider2D", "Node" }, { "Collision2D", "Node" }, { "BoxCollider2D", "CollisionShape2D" },
				{ "CircleCollider2D", "CollisionShape2D" }, { "CapsuleCollider2D", "CollisionShape2D" },
				{ "PolygonCollider2D", "CollisionPolygon2D" }, { "Text", "Label" }, { "Image", "TextureRect" },
			};
			for (const auto &pair : pairs) {
				aliases[pair[0]] = pair[1];
			}
		}
		const String *mapped = aliases.getptr(p_name);
		return mapped ? *mapped : p_name;
	}

	bool is_exportable(const String &p_name) const {
		static HashSet<String> builtin;
		if (builtin.is_empty()) {
			const char *names[] = { "int", "float", "bool", "String", "StringName", "NodePath", "Vector2", "Vector2i", "Vector3", "Vector3i",
				"Vector4", "Vector4i", "Rect2", "Rect2i", "Color", "Transform2D", "Array", "Dictionary", "PackedStringArray",
				"PackedInt32Array", "PackedFloat32Array", "PackedVector2Array", "PackedColorArray" };
			for (const char *name : names) {
				builtin.insert(name);
			}
		}
		if (builtin.has(p_name) || local_types.has(p_name)) {
			return true;
		}
		if (ClassDB::class_exists(p_name)) {
			return ClassDB::is_parent_class(p_name, "Node") || ClassDB::is_parent_class(p_name, "Resource");
		}
		return ScriptServer::is_global_class(p_name);
	}

	static bool is_constructible_builtin(const String &p_name) {
		static HashSet<String> names;
		if (names.is_empty()) {
			const char *list[] = { "Vector2", "Vector2i", "Vector3", "Vector3i", "Vector4", "Vector4i", "Rect2", "Rect2i", "Color",
				"Transform2D", "String", "StringName", "NodePath", "Array", "Dictionary", "Callable", "PackedStringArray",
				"PackedInt32Array", "PackedFloat32Array", "PackedVector2Array", "PackedColorArray", "Plane", "Quaternion", "Basis" };
			for (const char *name : list) {
				names.insert(name);
			}
		}
		return names.has(p_name);
	}

	// Parses a type: Name(.Name)*[<T, ...>][[]]*[?]. Returns false (without consuming) if it isn't one.
	bool try_parse_type(TypeInfo &r_type) {
		const int start = pos;
		if (!is_ident()) {
			return false;
		}
		String name = tokens[pos++].text;
		while (is(".") && peek_token().type == Token::IDENT) {
			pos++;
			name = tokens[pos++].text; // Namespaces are ignored (UnityEngine.Vector2 -> Vector2).
		}
		Vector<TypeInfo> args;
		if (is("<")) {
			pos++;
			while (true) {
				TypeInfo arg;
				if (!try_parse_type(arg)) {
					pos = start;
					return false;
				}
				args.push_back(arg);
				if (accept(",")) {
					continue;
				}
				if (accept(">")) {
					break;
				}
				pos = start;
				return false;
			}
		}
		int array_rank = 0;
		while (is("[") && is_at(1, "]")) {
			pos += 2;
			array_rank++;
		}
		const bool nullable = accept("?");

		r_type = TypeInfo();
		if (name == "void") {
			r_type.is_void = true;
			r_type.hint = "void";
			return true;
		}
		if (name == "var") {
			r_type.is_var = true;
			return true;
		}
		const String mapped = map_type_name(name);
		r_type.name = mapped;
		auto simple = [](const TypeInfo &p_type) { return !p_type.hint.is_empty() && !p_type.hint.contains_char('['); };
		if ((name == "List" || name == "HashSet" || name == "Queue" || name == "Stack") && args.size() == 1) {
			r_type.name = "Array";
			r_type.hint = simple(args[0]) ? "Array[" + args[0].hint + "]" : "Array";
		} else if (name == "Dictionary" && args.size() == 2) {
			r_type.name = "Dictionary";
			r_type.hint = (simple(args[0]) && simple(args[1])) ? "Dictionary[" + args[0].hint + ", " + args[1].hint + "]" : "Dictionary";
		} else if (mapped == "Variant") {
			r_type.hint = "";
		} else {
			r_type.hint = mapped;
		}
		if (array_rank > 0) {
			const TypeInfo element = r_type;
			r_type.name = "Array";
			r_type.hint = (array_rank == 1 && simple(element)) ? "Array[" + element.hint + "]" : "Array";
		}
		if (nullable) {
			r_type.hint = ""; // Nullable value types are Variants.
		}
		r_type.exportable = !r_type.hint.is_empty() && (r_type.name == "Array" || r_type.name == "Dictionary" || is_exportable(r_type.name));
		return true;
	}

	TypeInfo parse_type() {
		TypeInfo type;
		if (!try_parse_type(type)) {
			fail(vformat("Expected a type, found \"%s\".", cur().text));
		}
		return type;
	}

	static String typed_name(const String &p_name, const TypeInfo &p_type) {
		return p_type.hint.is_empty() ? p_name : p_name + ": " + p_type.hint;
	}

	// --- Expressions ---

	static int binary_precedence(const String &p_op) {
		if (p_op == "??") {
			return 1;
		} else if (p_op == "||") {
			return 2;
		} else if (p_op == "&&") {
			return 3;
		} else if (p_op == "|") {
			return 4;
		} else if (p_op == "^") {
			return 5;
		} else if (p_op == "&") {
			return 6;
		} else if (p_op == "==" || p_op == "!=") {
			return 7;
		} else if (p_op == "<" || p_op == ">" || p_op == "<=" || p_op == ">=" || p_op == "is" || p_op == "as") {
			return 8;
		} else if (p_op == "<<" || p_op == ">>") {
			return 9;
		} else if (p_op == "+" || p_op == "-") {
			return 10;
		} else if (p_op == "*" || p_op == "/" || p_op == "%") {
			return 11;
		}
		return -1;
	}

	// Current binary operator, with `>>` made of two adjacent `>` (they are split for generics).
	String current_binary_op(int &r_length) const {
		r_length = 1;
		const Token &t = cur();
		if (t.type == Token::IDENT && (t.text == "is" || t.text == "as")) {
			return t.text;
		}
		if (t.type != Token::PUNCT) {
			return String();
		}
		if (t.text == ">" && peek_token().type == Token::PUNCT && peek_token().text == ">" && peek_token().line == t.line && peek_token().column == t.end_column) {
			if (is_at(2, "=")) {
				return String(); // `>>=` is an assignment.
			}
			r_length = 2;
			return ">>";
		}
		return binary_precedence(t.text) >= 0 ? t.text : String();
	}

	String parse_expression() {
		return parse_conditional();
	}

	String parse_conditional() {
		const String condition = parse_binary(1);
		if (is("?")) {
			pos++;
			const String a = parse_conditional();
			expect(":");
			const String b = parse_conditional();
			const String result = "(" + a + " if " + condition + " else " + b + ")";
			return (is_string(a) && is_string(b)) ? mark_string(result) : result;
		}
		return condition;
	}

	String parse_binary(int p_min_precedence) {
		String left = parse_unary();
		while (true) {
			int length = 1;
			const String op = current_binary_op(length);
			const int precedence = op.is_empty() ? -1 : binary_precedence(op);
			if (precedence < p_min_precedence) {
				return left;
			}
			pos += length;
			if (op == "is" || op == "as") {
				const TypeInfo type = parse_type();
				left = "(" + left + " " + op + " " + (type.hint.is_empty() ? String("Object") : type.hint.get_slicec('[', 0)) + ")";
				continue;
			}
			String right = parse_binary(precedence + 1);
			if (op == "??") {
				left = "(" + left + " if " + left + " != null else " + right + ")";
			} else if (op == "+" && (is_string(left) || is_string(right))) {
				// "Score: " + 10 -> ("Score: " + str(10))
				if (!is_string(left)) {
					left = "str(" + left + ")";
				}
				if (!is_string(right)) {
					right = "str(" + right + ")";
				}
				left = mark_string("(" + left + " + " + right + ")");
			} else {
				left = "(" + left + " " + op + " " + right + ")";
			}
		}
	}

	bool starts_unary_operand() const {
		const Token &t = cur();
		if (t.type == Token::IDENT) {
			return t.text != "is" && t.text != "as";
		}
		if (t.type == Token::NUMBER || t.type == Token::STRING || t.type == Token::INTERPOLATED) {
			return true;
		}
		return t.type == Token::PUNCT && (t.text == "(" || t.text == "!" || t.text == "~");
	}

	String parse_unary() {
		if (is("!")) {
			pos++;
			return "!" + parse_unary();
		}
		if (is("-")) {
			pos++;
			return "-" + parse_unary();
		}
		if (is("+")) {
			pos++;
			return parse_unary();
		}
		if (is("~")) {
			pos++;
			return "~" + parse_unary();
		}
		if (is("++") || is("--")) {
			fail("\"++\" and \"--\" can only be used as statements (for example \"i++;\").");
		}
		// Cast: (Type)value
		if (is("(")) {
			const int start = pos;
			pos++;
			TypeInfo type;
			if (try_parse_type(type) && accept(")") && starts_unary_operand() && !type.is_var) {
				const String value = parse_unary();
				if (type.hint == "int" || type.hint == "float" || type.hint == "bool" || type.hint == "String") {
					return type.hint + "(" + value + ")";
				}
				return type.hint.is_empty() ? value : "(" + value + " as " + type.hint.get_slicec('[', 0) + ")";
			}
			pos = start;
		}
		return parse_postfix(parse_primary());
	}

	// Generic arguments of a call: Name<T, U>(...). Returns false (without consuming) otherwise.
	bool try_parse_generic_call_args(Vector<TypeInfo> &r_args) {
		if (!is("<")) {
			return false;
		}
		const int start = pos;
		pos++;
		while (true) {
			TypeInfo type;
			if (!try_parse_type(type)) {
				pos = start;
				return false;
			}
			r_args.push_back(type);
			if (accept(",")) {
				continue;
			}
			if (accept(">") && is("(")) {
				return true;
			}
			pos = start;
			r_args.clear();
			return false;
		}
	}

	static String type_value(const TypeInfo &p_type) {
		return p_type.hint.is_empty() ? String("Object") : p_type.hint.get_slicec('[', 0);
	}

	String parse_arguments(const Vector<TypeInfo> &p_type_args = Vector<TypeInfo>()) {
		expect("(");
		Vector<String> args;
		for (const TypeInfo &type : p_type_args) {
			args.push_back(type_value(type)); // GetComponent<T>() -> GetComponent(T)
		}
		if (!is(")")) {
			while (true) {
				if (is("ref") || is("out") || is("in")) {
					fail("\"ref\", \"out\" and \"in\" arguments are not supported.");
				}
				if (is_ident() && is_at(1, ":")) {
					fail("Named arguments are not supported yet.");
				}
				args.push_back(parse_expression());
				if (!accept(",")) {
					break;
				}
			}
		}
		expect(")");
		return "(" + String(", ").join(args) + ")";
	}

	String parse_interpolated(const Token &p_token) {
		// $"Hello {name}!" -> ("Hello " + str(name) + "!")
		const String &text = p_token.text;
		Vector<String> parts;
		String literal;
		int i = 0;
		while (i < text.length()) {
			const char32_t c = text[i];
			if (c == '\\' && i + 1 < text.length()) {
				literal += c;
				literal += text[i + 1];
				i += 2;
				continue;
			}
			if (c == '{' && i + 1 < text.length() && text[i + 1] == '{') {
				literal += '{';
				i += 2;
				continue;
			}
			if (c == '}' && i + 1 < text.length() && text[i + 1] == '}') {
				literal += '}';
				i += 2;
				continue;
			}
			if (c == '{') {
				int depth = 1;
				int j = i + 1;
				String expression;
				while (j < text.length() && depth > 0) {
					if (text[j] == '{') {
						depth++;
					} else if (text[j] == '}') {
						depth--;
						if (depth == 0) {
							break;
						}
					}
					expression += text[j];
					j++;
				}
				// Format specifiers ({value:F2}) are ignored.
				int colon = -1;
				int paren = 0;
				for (int k = 0; k < expression.length(); k++) {
					if (expression[k] == '(' || expression[k] == '[') {
						paren++;
					} else if (expression[k] == ')' || expression[k] == ']') {
						paren--;
					} else if (expression[k] == ':' && paren == 0) {
						colon = k;
					}
				}
				if (colon >= 0 && expression.find_char('?') < 0) {
					expression = expression.substr(0, colon);
				}
				if (!literal.is_empty()) {
					parts.push_back("\"" + literal + "\"");
					literal = String();
				}
				parts.push_back("str(" + translate_sub_expression(expression, p_token) + ")");
				i = j + 1;
				continue;
			}
			literal += c;
			i++;
		}
		if (!literal.is_empty() || parts.is_empty()) {
			parts.push_back("\"" + literal + "\"");
		}
		return parts.size() == 1 ? parts[0] : "(" + String(" + ").join(parts) + ")";
	}

	String translate_sub_expression(const String &p_code, const Token &p_at) {
		Vector<OroScriptTranslator::Error> sub_errors;
		Lexer lexer(p_code, sub_errors);
		Vector<Token> sub_tokens = lexer.tokenize();
		for (Token &t : sub_tokens) {
			t.line = p_at.line;
			t.column = p_at.column;
		}
		const Vector<Token> saved_tokens = tokens;
		const int saved_pos = pos;
		tokens = sub_tokens;
		pos = 0;
		const String result = parse_expression();
		tokens = saved_tokens;
		pos = saved_pos;
		if (failed) {
			fail_pos = saved_pos;
		}
		return result;
	}

	String parse_primary() {
		const Token t = cur();
		switch (t.type) {
			case Token::NUMBER:
			case Token::STRING:
				pos++;
				return t.text;
			case Token::INTERPOLATED:
				pos++;
				return mark_string(parse_interpolated(t));
			case Token::END:
				fail("Unexpected end of file.");
				break;
			default:
				break;
		}
		if (t.type == Token::PUNCT) {
			if (t.text == "(") {
				pos++;
				const String inner = parse_expression();
				expect(")");
				return is_string(inner) ? mark_string("(" + inner + ")") : "(" + inner + ")";
			}
			fail(vformat("Unexpected \"%s\".", t.text));
		}

		// Identifiers and keywords.
		const String &word = t.text;
		if (word == "true" || word == "false" || word == "null") {
			pos++;
			return word;
		}
		if (word == "this") {
			pos++;
			return "self";
		}
		if (word == "base") {
			pos++;
			return "super";
		}
		if (word == "typeof") {
			pos++;
			expect("(");
			const TypeInfo type = parse_type();
			expect(")");
			return type_value(type);
		}
		if (word == "new") {
			pos++;
			const Token &type_token = cur();
			const TypeInfo type = parse_type();
			if (is("{")) {
				fail("Object and collection initializers are not supported yet.", type_token);
			}
			if (is("[")) {
				fail("\"new T[n]\" arrays are not supported yet. Use a List<T> or [].", type_token);
			}
			const String args = parse_arguments();
			if (type.name == "Array") {
				return args == "()" ? String("[]") : "Array" + args;
			}
			if (type.name == "Dictionary") {
				return args == "()" ? String("{}") : "Dictionary" + args;
			}
			if (is_constructible_builtin(type.name)) {
				return type.name + args;
			}
			return type.name + ".new" + args;
		}
		if (word == "default" || word == "await" || word == "async" || word == "stackalloc" || word == "checked") {
			fail(vformat("\"%s\" is not supported yet.", word));
		}

		pos++;
		Vector<TypeInfo> type_args;
		if (try_parse_generic_call_args(type_args)) {
			return word + parse_arguments(type_args);
		}
		return word;
	}

	String parse_postfix(String p_base) {
		while (true) {
			if (is(".")) {
				pos++;
				const String member = expect_ident();
				if (member == "ToString" && is("(") && is_at(1, ")")) {
					pos += 2;
					p_base = "str(" + p_base + ")";
					continue;
				}
				if ((member == "Count" || member == "Length") && !is("(")) {
					p_base = "len(" + p_base + ")"; // List.Count, array.Length, string.Length
					continue;
				}
				Vector<TypeInfo> type_args;
				if (try_parse_generic_call_args(type_args)) {
					p_base += "." + member + parse_arguments(type_args);
					continue;
				}
				p_base += "." + member;
			} else if (is("(")) {
				p_base += parse_arguments();
			} else if (is("[")) {
				pos++;
				const String index = parse_expression();
				expect("]");
				p_base += "[" + index + "]";
			} else if (is("?.")) {
				fail("\"?.\" is not supported yet.");
			} else {
				return p_base;
			}
		}
	}

	// --- Statements ---

	// Expression statement, assignment or increment (without the trailing ';').
	String parse_simple_statement() {
		if (is("++") || is("--")) {
			const bool increment = is("++");
			pos++;
			const String target = parse_unary();
			return target + (increment ? " += 1" : " -= 1");
		}
		const String target = parse_conditional();
		if (is("++") || is("--")) {
			const bool increment = is("++");
			pos++;
			return target + (increment ? " += 1" : " -= 1");
		}
		static const char *assignments[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", nullptr };
		for (int i = 0; assignments[i]; i++) {
			if (is(assignments[i])) {
				const String op = assignments[i];
				pos++;
				return target + " " + op + " " + parse_expression();
			}
		}
		if (is(">") && is_at(1, ">") && is_at(2, "=")) {
			pos += 3;
			return target + " >>= " + parse_expression();
		}
		if (is("??=")) {
			pos++;
			const String value = parse_expression();
			return "if " + target + " == null: " + target + " = " + value;
		}
		return target;
	}

	// Local variable declaration (Type a = 1, b;) if there is one here.
	bool try_parse_local_declaration(int p_indent, int p_line, bool p_constant) {
		const int start = pos;
		TypeInfo type;
		if (!try_parse_type(type) || !is_ident() || !(is_at(1, "=") || is_at(1, ";") || is_at(1, ","))) {
			pos = start;
			return false;
		}
		while (true) {
			const String name = expect_ident();
			String text = (p_constant ? "const " : "var ") + typed_name(name, type);
			if (accept("=")) {
				text += " = " + parse_expression();
			}
			emit(p_line, p_indent, text);
			if (!accept(",")) {
				break;
			}
		}
		expect(";");
		return true;
	}

	// Body of if/while/for: a block or a single statement. Returns the number of statements.
	void parse_body(int p_indent, int p_header_line) {
		if (is("{")) {
			parse_block(p_indent);
			return;
		}
		const int count = parse_statement(p_indent);
		if (count == 0) {
			emit(p_header_line, p_indent, "pass");
		}
	}

	void parse_block(int p_indent) {
		expect("{");
		int count = 0;
		while (!is("}") && !at_end()) {
			count += parse_statement(p_indent);
		}
		const int close_line = cur().line;
		expect("}");
		if (count == 0) {
			emit(close_line, p_indent, "pass");
		}
	}

	// Returns the number of statements emitted.
	int parse_statement(int p_indent) {
		const int start = pos;
		const int count = parse_statement_inner(p_indent);
		if (failed) {
			recover_if_failed(start);
			return 1;
		}
		return count;
	}

	int parse_statement_inner(int p_indent) {
		const Token first = cur();
		const int line = first.line;

		if (is(";")) {
			pos++;
			return 0;
		}
		if (is("{")) {
			emit(line, p_indent, "if true:", true);
			parse_block(p_indent + 1);
			return 1;
		}
		if (first.type == Token::IDENT) {
			const String &word = first.text;
			if (word == "if") {
				pos++;
				expect("(");
				const String condition = parse_expression();
				expect(")");
				emit(line, p_indent, "if " + condition + ":", true);
				parse_body(p_indent + 1, line);
				while (is("else")) {
					const int else_line = cur().line;
					pos++;
					if (is("if")) {
						pos++;
						expect("(");
						const String else_condition = parse_expression();
						expect(")");
						emit(else_line, p_indent, "elif " + else_condition + ":", true);
						parse_body(p_indent + 1, else_line);
					} else {
						emit(else_line, p_indent, "else:", true);
						parse_body(p_indent + 1, else_line);
						break;
					}
				}
				return 1;
			}
			if (word == "while") {
				pos++;
				expect("(");
				const String condition = parse_expression();
				expect(")");
				emit(line, p_indent, "while " + condition + ":", true);
				parse_body(p_indent + 1, line);
				return 1;
			}
			if (word == "do") {
				pos++;
				emit(line, p_indent, "while true:", true);
				parse_body(p_indent + 1, line);
				const int while_line = cur().line;
				if (!accept("while")) {
					fail("Expected \"while\" after the body of \"do\".");
				}
				expect("(");
				const String condition = parse_expression();
				expect(")");
				expect(";");
				emit(while_line, p_indent + 1, "if !(" + condition + "): break");
				return 1;
			}
			if (word == "for") {
				pos++;
				parse_for(p_indent, line);
				return 1;
			}
			if (word == "foreach") {
				pos++;
				expect("(");
				TypeInfo type = parse_type();
				const String name = expect_ident();
				if (!accept("in")) {
					fail("Expected \"in\" in \"foreach\".");
				}
				const String collection = parse_expression();
				expect(")");
				(void)type;
				emit(line, p_indent, "for " + name + " in " + collection + ":", true);
				parse_body(p_indent + 1, line);
				return 1;
			}
			if (word == "return") {
				pos++;
				String text = "return";
				if (!is(";")) {
					text += " " + parse_expression();
				}
				expect(";");
				emit(line, p_indent, text);
				return 1;
			}
			if (word == "break" || word == "continue") {
				pos++;
				expect(";");
				emit(line, p_indent, word);
				return 1;
			}
			if (word == "const") {
				pos++;
				if (!try_parse_local_declaration(p_indent, line, true)) {
					fail("Expected a constant declaration.");
				}
				return 1;
			}
			if (word == "switch" || word == "try" || word == "throw" || word == "goto" || word == "lock" || word == "using" || word == "yield" || word == "unsafe" || word == "fixed") {
				fail(vformat("\"%s\" is not supported yet.", word));
			}
			if (try_parse_local_declaration(p_indent, line, false)) {
				return 1;
			}
		}

		const String text = parse_simple_statement();
		expect(";");
		emit(line, p_indent, text);
		return 1;
	}

	void parse_for(int p_indent, int p_line) {
		expect("(");
		// Fast path: for (int i = a; i < b; i++) -> for i in range(a, b):
		const int start = pos;
		{
			TypeInfo type;
			String counter;
			String from;
			bool ok = false;
			{
				if (try_parse_type(type) && is_ident() && is_at(1, "=")) {
					counter = expect_ident();
					pos++;
					from = parse_expression();
					ok = accept(";");
				}
				String to;
				String op;
				if (ok && is_ident() && cur().text == counter && (is_at(1, "<") || is_at(1, "<=") || is_at(1, ">") || is_at(1, ">="))) {
					pos++;
					op = tokens[pos++].text;
					to = parse_binary(9); // Above comparisons.
					ok = accept(";");
				} else {
					ok = false;
				}
				String step;
				if (ok && is_ident() && cur().text == counter) {
					pos++;
					if (accept("++")) {
						step = "1";
					} else if (accept("--")) {
						step = "-1";
					} else if (accept("+=")) {
						step = parse_expression();
					} else if (accept("-=")) {
						step = "-(" + parse_expression() + ")";
					} else {
						ok = false;
					}
				} else if (ok && (is("++") || is("--")) && peek_token().type == Token::IDENT && peek_token().text == counter) {
					step = is("++") ? "1" : "-1";
					pos += 2;
				} else {
					ok = false;
				}
				ok = ok && accept(")");
				const bool up = ok && (op == "<" || op == "<=") && !step.begins_with("-");
				const bool down = ok && (op == ">" || op == ">=") && step.begins_with("-");
				if (up || down) {
					String end = to;
					if (op == "<=") {
						end = "(" + to + ") + 1";
					} else if (op == ">=") {
						end = "(" + to + ") - 1";
					}
					String range = "range(" + from + ", " + end;
					if (step != "1") {
						range += ", " + step;
					}
					range += ")";
					emit(p_line, p_indent, "for " + counter + " in " + range + ":", true);
					parse_body(p_indent + 1, p_line);
					return;
				}
			}
			if (failed) {
				// Not the simple form: forget the error and try the general form.
				failed = false;
				errors.remove_at(errors.size() - 1);
			}
		}

		// General form: init; while condition: body; iterator.
		pos = start;
		if (!is(";")) {
			if (!try_parse_local_declaration(p_indent, p_line, false)) {
				emit(p_line, p_indent, parse_simple_statement());
				expect(";");
			}
		} else {
			pos++;
		}
		const String condition = is(";") ? String("true") : parse_expression();
		expect(";");
		Vector<String> iterators;
		while (!is(")")) {
			iterators.push_back(parse_simple_statement());
			if (!accept(",")) {
				break;
			}
		}
		expect(")");
		emit(p_line, p_indent, "while " + condition + ":", true);
		if (is("{")) {
			pos++;
			while (!is("}") && !at_end()) {
				parse_statement(p_indent + 1);
			}
			const int close_line = cur().line;
			expect("}");
			for (const String &iterator : iterators) {
				emit(close_line, p_indent + 1, iterator);
			}
		} else {
			parse_statement(p_indent + 1);
			for (const String &iterator : iterators) {
				emit(p_line, p_indent + 1, iterator);
			}
		}
	}

	// --- Declarations ---

	struct Attribute {
		String name;
		String args; // Translated, with parentheses.
		int line = 1;
	};

	Vector<Attribute> parse_attributes() {
		Vector<Attribute> attributes;
		while (is("[")) {
			pos++;
			while (true) {
				Attribute attribute;
				attribute.line = cur().line;
				attribute.name = expect_ident();
				while (accept(".")) {
					attribute.name = expect_ident();
				}
				if (attribute.name.ends_with("Attribute")) {
					attribute.name = attribute.name.trim_suffix("Attribute");
				}
				if (is("(")) {
					attribute.args = parse_arguments();
				}
				attributes.push_back(attribute);
				if (!accept(",")) {
					break;
				}
			}
			expect("]");
		}
		return attributes;
	}

	static bool is_modifier(const String &p_word) {
		static HashSet<String> modifiers;
		if (modifiers.is_empty()) {
			const char *list[] = { "public", "private", "protected", "internal", "static", "readonly", "const", "override", "virtual",
				"abstract", "sealed", "new", "partial", "extern", "unsafe", "volatile", "async" };
			for (const char *m : list) {
				modifiers.insert(m);
			}
		}
		return modifiers.has(p_word);
	}

	void parse_member(int p_indent, const String &p_class_name, int &r_count) {
		const int start = pos;
		parse_member_inner(p_indent, p_class_name, r_count);
		recover_if_failed(start);
	}

	void parse_member_inner(int p_indent, const String &p_class_name, int &r_count) {
		const Vector<Attribute> attributes = parse_attributes();
		HashSet<String> modifiers;
		while (is_ident() && is_modifier(cur().text) && !(cur().text == "new" && is_at(1, "("))) {
			modifiers.insert(tokens[pos++].text);
		}
		const Token first = cur();

		if (is("class") || is("struct")) {
			parse_class(p_indent, false);
			r_count++;
			return;
		}
		if (is("enum")) {
			parse_enum(p_indent);
			r_count++;
			return;
		}
		if (is("interface") || is("delegate") || is("event") || is("operator") || is("implicit") || is("explicit")) {
			fail(vformat("\"%s\" is not supported yet.", first.text));
		}

		// Constructor.
		if (is_ident() && cur().text == p_class_name && is_at(1, "(")) {
			pos++;
			parse_method(p_indent, first.line, "_init", TypeInfo(), modifiers);
			r_count++;
			return;
		}

		const TypeInfo type = parse_type();
		const Token name_token = cur();
		const String name = expect_ident();

		if (is("(")) {
			parse_method(p_indent, name_token.line, name, type, modifiers);
			r_count++;
			return;
		}
		if (is("{") || is("=>")) {
			fail("Properties ({ get; set; }) are not supported yet. Use a field.", name_token);
		}

		// Field(s).
		bool serialize_field = false;
		bool hidden = false;
		Vector<String> annotations;
		for (const Attribute &attribute : attributes) {
			if (attribute.name == "SerializeField" || attribute.name == "Export") {
				serialize_field = true;
			} else if (attribute.name == "HideInInspector" || attribute.name == "NonSerialized") {
				hidden = true;
			} else if (attribute.name == "Header") {
				emit(attribute.line, p_indent, "@export_group" + attribute.args);
			} else if (attribute.name == "Range") {
				annotations.push_back("@export_range" + attribute.args);
			} else if (attribute.name == "TextArea" || attribute.name == "Multiline") {
				annotations.push_back("@export_multiline");
			}
		}
		const bool is_const = modifiers.has("const");
		const bool is_static = modifiers.has("static");
		const bool exported = !is_const && !is_static && !hidden && (modifiers.has("public") || serialize_field) && type.exportable;

		while (true) {
			String text;
			if (exported) {
				text = annotations.is_empty() ? String("@export ") : String(" ").join(annotations) + " ";
			}
			if (is_const) {
				text += "const ";
			} else {
				text += is_static ? "static var " : "var ";
			}
			const String field_name = text.is_empty() ? name : name;
			(void)field_name;
			text += typed_name(name, type);
			if (accept("=")) {
				text += " = " + parse_expression();
			}
			emit(name_token.line, p_indent, text);
			r_count++;
			if (!accept(",")) {
				break;
			}
			// int a, b; -> the next declarator.
			const Token next = cur();
			const String next_name = expect_ident();
			String next_text = exported ? String("@export ") : String();
			next_text += is_const ? "const " : (is_static ? "static var " : "var ");
			next_text += typed_name(next_name, type);
			if (accept("=")) {
				next_text += " = " + parse_expression();
			}
			emit(next.line, p_indent, next_text);
			r_count++;
			if (!accept(",")) {
				break;
			}
			fail("Declare at most two fields per line.");
			break;
		}
		expect(";");
	}

	void parse_method(int p_indent, int p_line, const String &p_name, const TypeInfo &p_return, const HashSet<String> &p_modifiers) {
		expect("(");
		Vector<String> params;
		if (!is(")")) {
			while (true) {
				parse_attributes();
				if (is("ref") || is("out") || is("in") || is("params") || is("this")) {
					fail(vformat("\"%s\" parameters are not supported yet.", cur().text));
				}
				const TypeInfo type = parse_type();
				String param = typed_name(expect_ident(), type);
				if (accept("=")) {
					param += " = " + parse_expression();
				}
				params.push_back(param);
				if (!accept(",")) {
					break;
				}
			}
		}
		expect(")");
		String header = p_modifiers.has("static") ? "static func " : "func ";
		header += p_name + "(" + String(", ").join(params) + ")";
		if (!p_return.hint.is_empty() && p_name != "_init") {
			header += " -> " + p_return.hint;
		}
		header += ":";
		emit(p_line, p_indent, header, true);

		if (is("{")) {
			parse_block(p_indent + 1);
		} else if (accept("=>")) {
			const int line = cur().line;
			const String value = parse_simple_statement();
			expect(";");
			emit(line, p_indent + 1, p_return.is_void || p_return.hint.is_empty() && p_name == "_init" ? value : "return " + value);
		} else {
			const int line = cur().line;
			expect(";"); // abstract / extern
			emit(line, p_indent + 1, "pass");
		}
	}

	void parse_enum(int p_indent) {
		const int line = cur().line;
		pos++; // enum
		const String name = expect_ident();
		if (accept(":")) {
			parse_type(); // Underlying type.
		}
		expect("{");
		Vector<String> values;
		while (!is("}") && !at_end()) {
			parse_attributes();
			String value = expect_ident();
			if (accept("=")) {
				value += " = " + parse_expression();
			}
			values.push_back(value);
			if (!accept(",")) {
				break;
			}
		}
		expect("}");
		accept(";");
		local_types.insert(name);
		emit(line, p_indent, "enum " + name + " { " + String(", ").join(values) + " }");
	}

	void parse_class(int p_indent, bool p_top_level) {
		const int line = cur().line;
		const bool is_struct = is("struct");
		pos++; // class / struct
		const String name = expect_ident();
		if (is("<")) {
			fail("Generic classes are not supported yet.");
		}
		String base = "RefCounted";
		if (accept(":")) {
			const TypeInfo base_type = parse_type();
			// Interfaces after the base class are ignored.
			while (accept(",")) {
				parse_type();
			}
			if (!base_type.name.is_empty()) {
				base = base_type.name;
			}
		}
		(void)is_struct;
		while (is("where")) {
			fail("Generic constraints are not supported yet.");
		}
		local_types.insert(name);

		int member_indent = p_indent;
		if (p_top_level) {
			if (ClassDB::class_exists(name)) {
				emit(line, p_indent, "extends " + base); // Can't hide an engine class.
			} else {
				emit(line, p_indent, "class_name " + name + " extends " + base);
			}
		} else {
			emit(line, p_indent, "class " + name + " extends " + base + ":", true);
			member_indent = p_indent + 1;
		}

		expect("{");
		int count = 0;
		while (!is("}") && !at_end()) {
			parse_member(member_indent, name, count);
		}
		const int close_line = cur().line;
		expect("}");
		accept(";");
		if (!p_top_level && count == 0) {
			emit(close_line, member_indent, "pass");
		}
	}

	void parse_namespace_item() {
		if (accept("using")) {
			while (!is(";") && !at_end()) {
				pos++;
			}
			expect(";");
			return;
		}
		if (accept("namespace")) {
			expect_ident();
			while (accept(".")) {
				expect_ident();
			}
			if (accept("{")) {
				parse_namespace_body(true);
				expect("}");
			} else {
				expect(";"); // File-scoped namespace.
			}
			return;
		}
		// A type declaration.
		const int decl_start = pos;
		parse_attributes();
		while (is_ident() && is_modifier(cur().text)) {
			pos++;
		}
		if (is("class") || is("struct")) {
			// The first class is the script's class; the next ones become inner classes.
			parse_class(0, !has_top_class);
			has_top_class = true;
			return;
		}
		if (is("enum")) {
			parse_enum(0);
			return;
		}
		pos = decl_start;
		fail(vformat("Expected a class declaration, found \"%s\".", cur().text));
	}

	void parse_namespace_body(bool p_braced) {
		while (!at_end() && !(p_braced && is("}"))) {
			const int start = pos;
			parse_namespace_item();
			recover_if_failed(start);
		}
	}

public:
	Translator(const Vector<Token> &p_tokens, int p_line_count, Vector<OroScriptTranslator::Error> &r_errors) :
			tokens(p_tokens), errors(r_errors) {
		(void)out.resize(MAX(p_line_count, tokens[tokens.size() - 1].line) + 1);
	}

	String run() {
		parse_namespace_body(false);

		String result;
		for (int i = 0; i < out.size(); i++) {
			const Vector<OutLine> &slot = out[i];
			if (slot.is_empty()) {
				result += "\n";
				continue;
			}
			for (const OutLine &line : slot) {
				result += String("\t").repeat(line.indent) + line.text + "\n";
			}
		}
		return result;
	}
};

} // namespace

String OroScriptTranslator::translate(const String &p_source, Vector<Error> &r_errors) {
	Lexer lexer(p_source, r_errors);
	const Vector<Token> tokens = lexer.tokenize();
	if (!r_errors.is_empty()) {
		return String();
	}
	const int line_count = p_source.count("\n") + 1;
	Translator translator(tokens, line_count, r_errors);
	return translator.run();
}
