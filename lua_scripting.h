#ifndef MINIMALREDIS_LUA_SCRIPTING_H
#define MINIMALREDIS_LUA_SCRIPTING_H

#include <string>
#include <unordered_map>
#include <vector>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class Database;

class LuaScriptingEngine {
private:
    lua_State* L;
    Database& db;
    std::unordered_map<std::string, std::string> script_cache;

    static int redis_call(lua_State* L);
    static int redis_pcall(lua_State* L);
    static int redis_error_reply(lua_State* L);
    static int redis_status_reply(lua_State* L);
    static int redis_log(lua_State* L);

    void setup_redis_api();
    std::string calculate_sha1(const std::string& script);
    bool validate_script(const std::string& script);

public:
    LuaScriptingEngine(Database& database);
    ~LuaScriptingEngine();

    std::string eval(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args);
    std::string evalsha(const std::string& sha1, const std::vector<std::string>& keys, const std::vector<std::string>& args);
    bool script_exists(const std::string& sha1);
    std::string script_load(const std::string& script);
    void script_flush();
};

#endif //MINIMALREDIS_LUA_SCRIPTING_H