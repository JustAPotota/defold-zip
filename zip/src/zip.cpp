// myextension.cpp
// Extension lib defines
#define LIB_NAME "Zip"
#define MODULE_NAME "zip"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <zip.h>

struct ZipFile
{
    dmZip::HZip* zip;
    bool open;
};

static dmZip::HZip* ToZipFile(lua_State* L, int index)
{
    ZipFile* zipfile = (ZipFile*)lua_touserdata(L, index);
    if (zipfile->open)
    {
        return zipfile->zip;
    }
    else
    {
        luaL_error(L, "attempt to use a closed zip file");
        return NULL;
    }
}

static int ZipOpen(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    const char* path = (char*)luaL_checkstring(L, 1);

    ZipFile* zipfile = (ZipFile*)lua_newuserdata(L, sizeof(ZipFile));

    zipfile->zip = (dmZip::HZip*)malloc(sizeof(dmZip::HZip));

    dmZip::Result result = dmZip::Open(path, zipfile->zip);
    if (result != dmZip::RESULT_OK)
        return luaL_error(L, "Zip file '%s' not found", path);

    zipfile->open = true;

    luaL_getmetatable(L, "zipfile");
    lua_setmetatable(L, -2);

    return 1;
}

static int ZipFileOpenEntry(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmZip::HZip* zip = ToZipFile(L, 1);

    dmZip::Result result;
    if (lua_isnumber(L, 2)) {
        uint32_t entry_index = lua_tointeger(L, 2);
        dmZip::Result result = dmZip::OpenEntry(*zip, entry_index);
        if (result != dmZip::RESULT_OK)
            return luaL_error(L, "no entry at index %d", entry_index);
    } else {
        const char* entry_name = luaL_checkstring(L, 2);
        dmZip::Result result = dmZip::OpenEntry(*zip, entry_name);
        if (result != dmZip::RESULT_OK)
            return luaL_error(L, "no entry with name '%s'", entry_name);
    }

    return 0;
}

static int ZipFileCloseEntry(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    dmZip::HZip* zip = ToZipFile(L, 1);
    dmZip::CloseEntry(*zip);

    return 0;
}

static int ZipFileGetEntryName(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dmZip::HZip* zip = ToZipFile(L, 1);
    lua_pushstring(L, dmZip::GetEntryName(*zip));

    return 1;
}

static int ZipFileReadEntry(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dmZip::HZip* zip = ToZipFile(L, 1);

    uint32_t entry_size;
    dmZip::GetEntrySize(*zip, &entry_size);

    char* buffer = (char*)malloc(entry_size);
    dmZip::GetEntryData(*zip, buffer, entry_size);

    lua_pushlstring(L, buffer, entry_size);
    free(buffer);

    return 1;
}

static int ZipFileClose(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ZipFile* zipfile = (ZipFile*)lua_touserdata(L, 1);

    if (zipfile->open)
    {
        dmZip::Close(*zipfile->zip);
        zipfile->open = false;
    }

    return 0;
}

static int ZipFileLength(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    dmZip::HZip* zip = ToZipFile(L, 1);
    uint32_t entry_count = dmZip::GetNumEntries(*zip);

    lua_pushnumber(L, entry_count);

    return 1;
}

// Functions exposed to Lua
static const luaL_reg MODULE_METHODS[] =
{
    {"open", ZipOpen},
    {0, 0}
};

static const luaL_reg ZIPFILE_META[] =
{
    {"__len", ZipFileLength},
    {"__gc", ZipFileClose},
    {"close", ZipFileClose},
    {"open_entry", ZipFileOpenEntry},
    {"close_entry", ZipFileCloseEntry},
    {"get_entry_name", ZipFileGetEntryName},
    {"read_entry", ZipFileReadEntry},
    {0, 0}
};

// https://github.com/lua/lua/blob/6baee9ef9d5657ab582c8a4b9f885ec58ed502d0/lauxlib.c#L928
static void luaL53_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        if (l->func == NULL)  /* place holder? */
        lua_pushboolean(L, 0);
        else {
            int i;
            for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -nup);
            lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        }
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_pop(L, nup);  /* remove upvalues */
}

static void InitZipfileMetatable(lua_State* L)
{
    luaL_newmetatable(L, "zipfile");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL53_setfuncs(L, ZIPFILE_META, 0);
    lua_pop(L, 1);
}

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, MODULE_METHODS);

    InitZipfileMetatable(L);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result InitializeZipExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// MyExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(Zip, LIB_NAME, 0, 0, InitializeZipExtension, 0, 0, 0)
