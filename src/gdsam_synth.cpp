#include "gdsam_synth.h"

#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/binder_common.hpp>

extern "C" {
    #include "reciter.h"
    #include "sam.h"
    #include "debug.h"
}

using namespace godot;

extern "C" {
    int debug = 0;

    int8_t* _speak(const String phrase, 
        int speed, 
        int pitch, 
        int mouth, 
        int throat, 
        bool singing, 
        bool phonetic,
        int &buffer_size) {

        CharString char_string = phrase.utf8();
        const char *input = char_string.get_data();

        SetSpeed(speed & 0xFF);
        SetPitch(pitch & 0xFF);
        SetMouth(mouth & 0xFF);
        SetThroat(throat & 0xFF);
        
        if(singing) {
            EnableSingmode();
        }

        char _input[256];
        for(int i = 0; i < 256; i++) _input[i] = 0;
        strcpy(_input, input);
        for(int i = 0; _input[i] != 0; i++) _input[i] = toupper((int)_input[i]);

        if(phonetic) {
            strncat(_input, "[", 255);
            if(!TextToPhonemes((unsigned char *)_input)) {
                return NULL;
            }
        } else {
            strncat(_input, "\x9b", 255);
        }

         SetInput(_input);
        if (!SAMMain()) {
            return NULL;
        }

        int buffer_length = GetBufferLength() / 50;
        buffer_size = buffer_length;
        char *buffer = GetBuffer();
       
        int8_t *g_buffer = (int8_t*)malloc(sizeof(int8_t) * buffer_length);

        for(int i = 0; i < buffer_length; i++) {
            g_buffer[i] = buffer[i] - 128;
        }

        free(buffer);

        return g_buffer;
    }
}

GDSAMSynth::GDSAMSynth() {
    mouth = 64;
    pitch = 64;
    mouth = 128;
    throat = 128;
    singing = 0;
    phonetic = 0;
}

GDSAMSynth::~GDSAMSynth() {

}

void GDSAMSynth::set_speed(int speed) {
    this->speed = speed;
}

void GDSAMSynth::set_pitch(int pitch) {
    this->pitch = pitch;
}

void GDSAMSynth::set_mouth(int mouth) {
    this->mouth = mouth;
}

void GDSAMSynth::set_throat(int throat) {
    this->throat = throat;
}

void GDSAMSynth::set_singing(bool singing) {
    this->singing = singing;
}

void GDSAMSynth::set_phonetic(bool phonetic) {
    this->phonetic = phonetic;
}

PackedByteArray GDSAMSynth::speak(const String phrase) const {
    int buffer_size = 0;
    int8_t *g_buffer = _speak(phrase, this->speed, this->pitch, this->mouth, this->throat, this->singing, this->phonetic, buffer_size);
    PackedByteArray out = PackedByteArray();
    if (g_buffer) {
        for(int i = 0; i < buffer_size; i++) {
            out.append(g_buffer[i]);
        }
    }

    delete g_buffer;

    return out;
}

void GDSAMSynth::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_speed"), &GDSAMSynth::set_speed);
    ClassDB::bind_method(D_METHOD("set_pitch"), &GDSAMSynth::set_pitch);
    ClassDB::bind_method(D_METHOD("set_mouth"), &GDSAMSynth::set_mouth);
    ClassDB::bind_method(D_METHOD("set_throat"), &GDSAMSynth::set_throat);
    ClassDB::bind_method(D_METHOD("set_singing"), &GDSAMSynth::set_singing);
    ClassDB::bind_method(D_METHOD("set_phonetic"), &GDSAMSynth::set_phonetic);
    ClassDB::bind_method(D_METHOD("speak"), &GDSAMSynth::speak);
}

