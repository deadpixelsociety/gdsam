#ifndef GDSAM_CLASS_H
#define GDSAM_CLASS_H

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/binder_common.hpp>

using namespace godot;

class GDSAMSynth : public RefCounted {
    GDCLASS(GDSAMSynth, RefCounted);

    protected:
        static void _bind_methods();

    private:
        int speed;
        int pitch;
        int mouth;
        int throat;
        bool singing;
        bool phonetic;

    public:
        GDSAMSynth();
        ~GDSAMSynth();

        void set_speed(int speed);
        void set_pitch(int pitch);
        void set_mouth(int mouth);
        void set_throat(int throat);
        void set_singing(bool singing);
        void set_phonetic(bool phonetic);
        PackedByteArray speak(const String phrase) const;
};

#endif GDSAM_CLASS_H