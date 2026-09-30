---@meta
-- LuaLS definitions for the Premake5 API, used only for editor hover/completion.
-- Nothing here is executed by premake.
--
-- Hand-written and intentionally limited to what the arcade scripts use.
-- Premake has no official LuaLS definitions (premake-core issue #2724 was closed).
-- Verify signatures against https://premake.github.io/docs/ and add functions
-- here when LuaLS reports an undefined global/field.

---@param name string
function workspace(name) end

---@param name string
function project(name) end

---@param name string
function group(name) end

---@param kind "ConsoleApp"|"WindowedApp"|"StaticLib"|"SharedLib"|"Makefile"|"Utility"|"None"
function kind(kind) end

---@param lang "C"|"C++"|"C#"
function language(lang) end

---@param dialect string e.g. "C++17"
function cppdialect(dialect) end

---@param dialect string e.g. "C99"
function cdialect(dialect) end

---@param patterns string|string[]
function files(patterns) end

---@param patterns string|string[]
function removefiles(patterns) end

---@param dirs string|string[]
function includedirs(dirs) end

---@param dirs string|string[]
function libdirs(dirs) end

---@param libs string|string[]
function links(libs) end

---@param defs string|string[]
function defines(defs) end

---@param dir string
function location(dir) end

---@param dir string
function targetdir(dir) end

---@param dir string
function objdir(dir) end

---@param name string
function targetname(name) end

---@param dir string
function debugdir(dir) end

---@param names string|string[]
function dependson(names) end

---@param opts string|string[]
function buildoptions(opts) end

---@param opts string|string[]
function linkoptions(opts) end

---@param names string|string[]
function configurations(names) end

---@param names string|string[]
function platforms(names) end

---@param arch string
function architecture(arch) end

---@param value string
function optimize(value) end

---@param value string
function symbols(value) end

---@param value string
function warnings(value) end

---@param value string
function staticruntime(value) end

---@param value string
function runtime(value) end

---@param value string
function systemversion(value) end

---@param value string
function toolset(value) end

---@param flags string|string[]
function flags(flags) end

---@param terms string|string[]|nil Filter terms; call filter() with no args to reset.
function filter(terms) end

---@param spec table Fields: trigger, value, description, allowed, default
function newoption(spec) end

---@param file string
function include(file) end

---@param file string
function includeexternal(file) end

---@type string
_ACTION = ""

---@type table<string, string>
_OPTIONS = {}

---@type string
_MAIN_SCRIPT_DIR = ""

---@type string
_SCRIPT_DIR = ""

---@class premake
premake = {}

---@param fmt string
---@param ... any
function printf(fmt, ...) end

---@class http
http = {}

---@param url string
---@param file string
---@param options table?
---@return string? body
---@return string|number? err
function http.download(url, file, options) end

-- ---------------------------------------------------------------------------
-- Premake extensions to the standard `os` table
-- ---------------------------------------------------------------------------

---@param p string
---@return boolean
function os.isdir(p) end

---@param p string
---@return boolean
function os.isfile(p) end

---@param p string
---@return boolean
function os.exists(p) end

---@param p string
---@return boolean ok
---@return string? err
function os.chdir(p) end

---@return string
function os.getcwd() end

---@param p string
---@return boolean ok
---@return string? err
function os.mkdir(p) end

---@param p string
---@return boolean ok
---@return string? err
function os.rmdir(p) end

---@param src string
---@param dst string
---@return boolean ok
---@return string? err
function os.copyfile(src, dst) end

---@param mask string
---@return string[]
function os.matchfiles(mask) end

---@param mask string
---@return string[]
function os.matchdirs(mask) end

---@param cmd string
---@return string? output
---@return number? code
function os.outputof(cmd) end

---@param id string e.g. "windows", "linux", "macosx"
---@return boolean
function os.istarget(id) end

---@return string
function os.target() end

---@return string
function os.host() end

---@param id string
---@return boolean
function os.ishost(id) end

-- ---------------------------------------------------------------------------
-- zip
-- ---------------------------------------------------------------------------

---@class zip
zip = {}

---@param archive string
---@param dest string
function zip.extract(archive, dest) end

-- ---------------------------------------------------------------------------
-- path
-- ---------------------------------------------------------------------------

---@class path
path = path or {}

---@param ... string
---@return string
function path.join(...) end

---@param p string
---@return string
function path.getabsolute(p) end

---@param p string
---@param rel string
---@return string
function path.getrelative(p, rel) end

---@param p string
---@return string
function path.getname(p) end

---@param p string
---@return string
function path.getdirectory(p) end

---@param p string
---@return string
function path.getextension(p) end

---@param p string
---@return string
function path.getbasename(p) end

---@param p string
---@return string
function path.translate(p) end

-- ---------------------------------------------------------------------------
-- Additional project/configuration APIs
-- ---------------------------------------------------------------------------

---@param name string Default platform for the workspace, e.g. "x64"
function defaultplatform(name) end

---@param value "On"|"Off"|"Default" Link-time optimization
function linktimeoptimization(value) end

---@param value "Default"|"ASCII"|"MBCS"|"Unicode"
function characterset(value) end

---@param value "Default"|"C"|"C++"|"Objective-C"|"Objective-C++"
function compileas(value) end

---@param warnings string|string[] Warnings to disable, e.g. "4996"
function disablewarnings(warnings) end

---@param warnings string|string[]
function enablewarnings(warnings) end

---@param cmds string|string[]
function prebuildcommands(cmds) end

---@param cmds string|string[]
function postbuildcommands(cmds) end

---@param cmds string|string[]
function prelinkcommands(cmds) end

---@param name string
function startproject(name) end

---@param name string
function entrypoint(name) end

---@param value string|string[]
function rtti(value) end

---@param value string
function exceptionhandling(value) end

---@param value string
function vectorextensions(value) end

---@param value string
function floatingpoint(value) end

---@param dir string|string[]
function sysincludedirs(dir) end

---@param dir string|string[]
function syslibdirs(dir) end

---@param args string|string[]
function debugargs(args) end

---@param value string
function pic(value) end

---@param value string
function editandcontinue(value) end


---@param map table<string, string|string[]> Virtual path groups, keyed by group name
function vpaths(map) end

