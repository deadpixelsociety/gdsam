#!python
import os, subprocess

opts = Variables([], ARGUMENTS)

# Define the relative path to the Godot headers.
godot_headers_path = "godot-headers/"

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()
env["STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME"] = 1

# Define our options. Use future-proofed names for platforms.
platform_array = ["", "windows", "linuxbsd", "macos", "x11", "linux", "osx"]
opts.Add(EnumVariable("target", "Compilation target", "release", ["d", "debug", "r", "release"]))
opts.Add(EnumVariable("platform", "Compilation platform", "windows", platform_array))
opts.Add(EnumVariable("p", "Alias for 'platform'", "windows", platform_array))
opts.Add(BoolVariable("use_llvm", "Use the LLVM / Clang compiler", "no"))
opts.Add(PathVariable("target_path", "The path where the lib is installed.", "bin/"))
opts.Add(PathVariable("target_name", "The library name.", "gdsam", PathVariable.PathAccept))

# Only support 64-bit systems.
bits = 64

# Updates the environment with the option variables.
opts.Update(env)

# Process platform arguments.
if env["p"] != "":
    env["platform"] = env["p"]

if env["platform"] == "osx":
    env["platform"] = "macos"
elif env["platform"] in ("x11", "linux"):
    env["platform"] = "linuxbsd"

if env["platform"] == "":
    print("No valid target platform selected.")
    quit()

platform = env["platform"]

# Check our platform specifics.
if platform == "macos":
    if not env["use_llvm"]:
        env["use_llvm"] = "yes"
    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-g", "-O2", "-arch", "x86_64"])
        env.Append(LINKFLAGS=["-arch", "x86_64"])
    else:
        env.Append(CCFLAGS=["-g", "-O3", "-arch", "x86_64"])
        env.Append(LINKFLAGS=["-arch", "x86_64"])
elif platform == "linuxbsd":
    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-fPIC", "-g3", "-Og"])
    else:
        env.Append(CCFLAGS=["-fPIC", "-g", "-O3"])
elif platform == "windows":
    # This makes sure to keep the session environment variables
    # on Windows, so that you can run scons in a VS 2017 prompt
    # and it will find all the required tools.
    env = Environment(ENV=os.environ)
    opts.Update(env)

    env.Append(CCFLAGS=["-DWIN32", "-D_WIN32", "-D_WINDOWS", "-W3", "-GR", "-D_CRT_SECURE_NO_WARNINGS"])
    if env["target"] in ("debug", "d"):
        env.Append(CCFLAGS=["-EHsc", "-D_DEBUG", "-MDd"])
    else:
        env.Append(CCFLAGS=["-O2", "-EHsc", "-DNDEBUG", "-MD"])

if env["use_llvm"] == "yes":
    env["CC"] = "clang"
    env["CXX"] = "clang++"

# Make sure our library includes the Godot headers.
env.Append(CPPPATH=[".", godot_headers_path])

env["target_path"] += platform + "/"

# Make sure our library looks in the target path for any other
# libraries it may need. The path needs to be project-relative.
# We remove "project/" from the target path with "[8:]".
if platform == "windows":
    env.Append(LINKFLAGS=["-LIBPATH:" + env["target_path"]])
else:
    env.Append(LINKFLAGS=["-Wl,-rpath," + (env["target_path"])[8:]])
    # The capital L path is used when compiling, so it includes "project/".
    env.Append(LINKFLAGS=["-L" + env["target_path"]])

# Tweak this if you want to use different folders,
# or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.c")

library = env.SharedLibrary(target=env["target_path"] + env["target_name"], source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
