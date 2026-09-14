#!/usr/bin/env python
import os
import sys

# Keep validation builds separate from packaged libraries when requested.
output_dir = ARGUMENTS.pop("output_dir", "bin")
env = SConscript("godot-cpp/SConstruct")

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.c**")


if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{}/{}/gdsam.{}.framework/libgdsam.{}".format(
            output_dir,
            env["platform"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{}/{}/gdsam.{}{}".format(
            output_dir, env["platform"], env["target"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
