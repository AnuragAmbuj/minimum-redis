#include "lua_scripting.h"
#include "db.h"
#include <sstream>
#include <iostream>
#include <cstring>

LuaScriptingEngine::LuaScriptingEngine(Database& database) : db(database) {
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create Lua state");
    }

    luaL_openlibs(L);
    setup_redis_api();
}

LuaScriptingEngine::~LuaScriptingEngine() {
    if (L) {
        lua_close(L);
    }
}

void LuaScriptingEngine::setup_redis_api() {
    lua_newtable(L);

    lua_pushcfunction(L, redis_call);
    lua_setfield(L, -2, "call");

    lua_pushcfunction(L, redis_pcall);
    lua_setfield(L, -2, "pcall");

    lua_pushcfunction(L, redis_error_reply);
    lua_setfield(L, -2, "error_reply");

    lua_pushcfunction(L, redis_status_reply);
    lua_setfield(L, -2, "status_reply");

    lua_pushcfunction(L, redis_log);
    lua_setfield(L, -2, "log");

    lua_setglobal(L, "redis");

    lua_pushlightuserdata(L, &db);
    lua_setfield(L, LUA_REGISTRYINDEX, "database");
}

int LuaScriptingEngine::redis_call(lua_State* L) {
    return redis_pcall(L);
}

int LuaScriptingEngine::redis_pcall(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        lua_pushstring(L, "redis.call requires at least 1 argument");
        lua_error(L);
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "First argument to redis.call must be a string");
        lua_error(L);
        return 0;
    }

    const char* cmd = lua_tostring(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "database");
    Database* db = static_cast<Database*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (strcmp(cmd, "GET") == 0 && nargs >= 2) {
        const char* key = lua_tostring(L, 2);
        std::string value = db->get(key);
        if (value.empty()) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, value.c_str());
        }
        return 1;
    } else if (strcmp(cmd, "SET") == 0 && nargs >= 3) {
        const char* key = lua_tostring(L, 2);
        const char* value = lua_tostring(L, 3);
        db->set(key, value);
        lua_pushstring(L, "OK");
        return 1;
    }

    lua_pushstring(L, "Command not implemented in Lua scripting");
    lua_error(L);
    return 0;
}

int LuaScriptingEngine::redis_error_reply(lua_State* L) {
    lua_pushvalue(L, 1);
    return 1;
}

int LuaScriptingEngine::redis_status_reply(lua_State* L) {
    lua_pushvalue(L, 1);
    return 1;
}

int LuaScriptingEngine::redis_log(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    std::cout << "[LUA LOG] " << msg << std::endl;
    return 0;
}

std::string LuaScriptingEngine::calculate_sha1(const std::string& script) {
    size_t hash = 0;
    for (char c : script) {
        hash = hash * 31 + static_cast<size_t>(c);
    }
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

bool LuaScriptingEngine::validate_script(const std::string& script) {
    if (script.find("os.") != std::string::npos ||
        script.find("io.") != std::string::npos ||
        script.find("package.") != std::string::npos) {
        return false;
    }
    return true;
}

std::string LuaScriptingEngine::eval(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    if (!validate_script(script)) {
        return "-ERR Script contains dangerous functions\r\n";
    }

    if (luaL_loadstring(L, script.c_str()) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        lua_pop(L, 1);
        return std::string("-ERR ") + error + "\r\n";
    }

    lua_newtable(L);
    for (size_t i = 0; i < keys.size(); ++i) {
        lua_pushstring(L, keys[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }

    lua_newtable(L);
    for (size_t i = 0; i < args.size(); ++i) {
        lua_pushstring(L, args[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        lua_pop(L, 1);
        return std::string("-ERR ") + error + "\r\n";
    }

    std::string result;
    if (lua_isstring(L, -1)) {
        result = lua_tostring(L, -1);
    } else if (lua_isnumber(L, -1)) {
        result = std::to_string(lua_tonumber(L, -1));
    } else if (lua_isboolean(L, -1)) {
        result = lua_toboolean(L, -1) ? "true" : "false";
    } else if (lua_isnil(L, -1)) {
        result = "(nil)";
    } else {
        result = "(unknown type)";
    }

    lua_pop(L, 1);
    return "+" + result + "\r\n";
}

std::string LuaScriptingEngine::evalsha(const std::string& sha1, const std::vector<std::string>& keys, const std::vector<std::string>& args) {
    auto it = script_cache.find(sha1);
    if (it == script_cache.end()) {
        return "-NOSCRIPT No matching script. Please use EVAL.\r\n";
    }

    return eval(it->second, keys, args);
}

bool LuaScriptingEngine::script_exists(const std::string& sha1) {
    return script_cache.find(sha1) != script_cache.end();
}

std::string LuaScriptingEngine::script_load(const std::string& script) {
    if (!validate_script(script)) {
        return "-ERR Script contains dangerous functions\r\n";
    }

    std::string sha1 = calculate_sha1(script);
    script_cache[sha1] = script;
    return "+" + sha1 + "\r\n";
}

void LuaScriptingEngine::script_flush() {
    script_cache.clear();
}