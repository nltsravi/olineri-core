#ifndef VAANI_INTERNAL_JSON_HPP
#define VAANI_INTERNAL_JSON_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstdint>

namespace vaani {
namespace json {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type = JsonType::Null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Bool), bool_val(b) {}
    JsonValue(double d) : type(JsonType::Number), num_val(d) {}
    JsonValue(int i) : type(JsonType::Number), num_val(static_cast<double>(i)) {}
    JsonValue(int64_t i) : type(JsonType::Number), num_val(static_cast<double>(i)) {}
    JsonValue(const std::string& s) : type(JsonType::String), str_val(s) {}
    JsonValue(const char* s) : type(JsonType::String), str_val(s ? s : "") {}
    JsonValue(const std::vector<JsonValue>& a) : type(JsonType::Array), arr_val(a) {}
    JsonValue(const std::map<std::string, JsonValue>& o) : type(JsonType::Object), obj_val(o) {}

    bool is_null() const { return type == JsonType::Null; }
    bool is_bool() const { return type == JsonType::Bool; }
    bool is_number() const { return type == JsonType::Number; }
    bool is_string() const { return type == JsonType::String; }
    bool is_array() const { return type == JsonType::Array; }
    bool is_object() const { return type == JsonType::Object; }

    bool as_bool() const { return bool_val; }
    double as_double() const { return num_val; }
    int64_t as_int() const { return static_cast<int64_t>(num_val); }
    const std::string& as_string() const { return str_val; }
    const std::vector<JsonValue>& as_array() const { return arr_val; }
    const std::map<std::string, JsonValue>& as_object() const { return obj_val; }

    bool has_key(const std::string& key) const {
        if (!is_object()) return false;
        return obj_val.find(key) != obj_val.end();
    }

    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue null_val;
        if (!is_object()) return null_val;
        auto it = obj_val.find(key);
        if (it != obj_val.end()) return it->second;
        return null_val;
    }

    const JsonValue& operator[](size_t index) const {
        static const JsonValue null_val;
        if (!is_array() || index >= arr_val.size()) return null_val;
        return arr_val[index];
    }

    std::string dump() const {
        std::ostringstream ss;
        serialize(ss);
        return ss.str();
    }

private:
    void serialize(std::ostringstream& ss) const {
        switch (type) {
            case JsonType::Null: ss << "null"; break;
            case JsonType::Bool: ss << (bool_val ? "true" : "false"); break;
            case JsonType::Number: ss << num_val; break;
            case JsonType::String: {
                ss << '"';
                for (char c : str_val) {
                    if (c == '"') ss << "\\\"";
                    else if (c == '\\') ss << "\\\\";
                    else if (c == '\b') ss << "\\b";
                    else if (c == '\f') ss << "\\f";
                    else if (c == '\n') ss << "\\n";
                    else if (c == '\r') ss << "\\r";
                    else if (c == '\t') ss << "\\t";
                    else ss << c;
                }
                ss << '"';
                break;
            }
            case JsonType::Array: {
                ss << '[';
                for (size_t i = 0; i < arr_val.size(); ++i) {
                    if (i > 0) ss << ", ";
                    arr_val[i].serialize(ss);
                }
                ss << ']';
                break;
            }
            case JsonType::Object: {
                ss << '{';
                bool first = true;
                for (const auto& kv : obj_val) {
                    if (!first) ss << ", ";
                    first = false;
                    ss << '"' << kv.first << "\": ";
                    kv.second.serialize(ss);
                }
                ss << '}';
                break;
            }
        }
    }
};

class JsonParser {
public:
    static JsonValue parse(const std::string& src) {
        size_t idx = 0;
        skip_ws(src, idx);
        JsonValue val = parse_val(src, idx);
        skip_ws(src, idx);
        return val;
    }

private:
    static void skip_ws(const std::string& src, size_t& idx) {
        while (idx < src.size() && (src[idx] == ' ' || src[idx] == '\t' || src[idx] == '\n' || src[idx] == '\r')) {
            idx++;
        }
    }

    static JsonValue parse_val(const std::string& src, size_t& idx) {
        skip_ws(src, idx);
        if (idx >= src.size()) return JsonValue();
        char c = src[idx];
        if (c == '{') return parse_obj(src, idx);
        if (c == '[') return parse_arr(src, idx);
        if (c == '"') return parse_str(src, idx);
        if (c == 't' || c == 'f') return parse_bool(src, idx);
        if (c == 'n') return parse_null(src, idx);
        if (c == '-' || (c >= '0' && c <= '9')) return parse_num(src, idx);
        return JsonValue();
    }

    static JsonValue parse_obj(const std::string& src, size_t& idx) {
        idx++; // skip '{'
        std::map<std::string, JsonValue> obj;
        skip_ws(src, idx);
        if (idx < src.size() && src[idx] == '}') {
            idx++;
            return JsonValue(obj);
        }
        while (idx < src.size()) {
            skip_ws(src, idx);
            if (idx >= src.size() || src[idx] != '"') break;
            std::string key = parse_str(src, idx).as_string();
            skip_ws(src, idx);
            if (idx >= src.size() || src[idx] != ':') break;
            idx++; // skip ':'
            JsonValue val = parse_val(src, idx);
            obj[key] = val;
            skip_ws(src, idx);
            if (idx < src.size() && src[idx] == ',') {
                idx++;
                continue;
            }
            if (idx < src.size() && src[idx] == '}') {
                idx++;
                break;
            }
        }
        return JsonValue(obj);
    }

    static JsonValue parse_arr(const std::string& src, size_t& idx) {
        idx++; // skip '['
        std::vector<JsonValue> arr;
        skip_ws(src, idx);
        if (idx < src.size() && src[idx] == ']') {
            idx++;
            return JsonValue(arr);
        }
        while (idx < src.size()) {
            JsonValue val = parse_val(src, idx);
            arr.push_back(val);
            skip_ws(src, idx);
            if (idx < src.size() && src[idx] == ',') {
                idx++;
                continue;
            }
            if (idx < src.size() && src[idx] == ']') {
                idx++;
                break;
            }
        }
        return JsonValue(arr);
    }

    static JsonValue parse_str(const std::string& src, size_t& idx) {
        idx++; // skip '"'
        std::string s;
        while (idx < src.size() && src[idx] != '"') {
            if (src[idx] == '\\' && idx + 1 < src.size()) {
                idx++;
                char esc = src[idx];
                if (esc == '"') s.push_back('"');
                else if (esc == '\\') s.push_back('\\');
                else if (esc == 'n') s.push_back('\n');
                else if (esc == 'r') s.push_back('\r');
                else if (esc == 't') s.push_back('\t');
                else s.push_back(esc);
            } else {
                s.push_back(src[idx]);
            }
            idx++;
        }
        if (idx < src.size() && src[idx] == '"') idx++;
        return JsonValue(s);
    }

    static JsonValue parse_bool(const std::string& src, size_t& idx) {
        if (src.compare(idx, 4, "true") == 0) {
            idx += 4;
            return JsonValue(true);
        }
        if (src.compare(idx, 5, "false") == 0) {
            idx += 5;
            return JsonValue(false);
        }
        return JsonValue();
    }

    static JsonValue parse_null(const std::string& src, size_t& idx) {
        if (src.compare(idx, 4, "null") == 0) {
            idx += 4;
        }
        return JsonValue();
    }

    static JsonValue parse_num(const std::string& src, size_t& idx) {
        size_t start = idx;
        if (src[idx] == '-') idx++;
        while (idx < src.size() && ((src[idx] >= '0' && src[idx] <= '9') || src[idx] == '.' || src[idx] == 'e' || src[idx] == 'E' || src[idx] == '+' || src[idx] == '-')) {
            idx++;
        }
        std::string num_str = src.substr(start, idx - start);
        try {
            double d = std::stod(num_str);
            return JsonValue(d);
        } catch (...) {
            return JsonValue(0.0);
        }
    }
};

inline JsonValue parse(const std::string& json_text) {
    return JsonParser::parse(json_text);
}

} // namespace json
} // namespace vaani

#endif // VAANI_INTERNAL_JSON_HPP
