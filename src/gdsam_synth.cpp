#include "gdsam_synth.h"

#include <cctype>
#include <mutex>
#include <cstdlib>
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

extern "C" { int debug = 0; }

// The inherited C engine has shared state; one utterance owns it at a time.
static std::mutex synthesis_mutex;
struct SamCleanup {
    ~SamCleanup() { FreeSam(); }
};

GDSAMSynth::GDSAMSynth() {
    speed = 72;
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

String GDSAMSynth::get_last_error() const {
    std::lock_guard<std::mutex> lock(synthesis_mutex);
    return last_error;
}

PackedByteArray GDSAMSynth::speak(const String phrase) const {
    std::lock_guard<std::mutex> lock(synthesis_mutex);
    SamCleanup cleanup;
    PackedByteArray out;
    last_error = String();
    ResetSam();
    SetSpeed(speed & 0xFF);
    SetPitch(pitch & 0xFF);
    SetMouth(mouth & 0xFF);
    SetThroat(throat & 0xFF);
    if (singing) EnableSingmode();

    CharString bytes = phrase.utf8();
    const char *text = bytes.get_data();
    size_t length = static_cast<size_t>(bytes.length());
    char *recited = nullptr;
    bool ok;
    if (phonetic) {
        ok = SetInputText(text, length) != 0;
    } else {
        size_t recited_length = 0;
        ok = ReciteText(text, length, &recited, &recited_length) != 0;
        if (ok) ok = SetInputText(recited, recited_length) != 0;
        std::free(recited);
    }
    if (ok) ok = SAMMain() != 0;
    if (!ok) {
        last_error = String::utf8(SamGetError());
        UtilityFunctions::push_error(last_error);
        return out;
    }

    int64_t length_bytes = static_cast<int64_t>(GetBufferLength() / 50);
    if (out.resize(length_bytes) != OK) {
        last_error = "Unable to allocate Godot audio buffer";
        UtilityFunctions::push_error(last_error);
        return PackedByteArray();
    }
    uint8_t *destination = out.ptrw();
    const unsigned char *source = reinterpret_cast<const unsigned char *>(GetBuffer());
    for (int64_t i = 0; i < length_bytes; ++i) destination[i] = source[i] - 128;
    return out;
}

void GDSAMSynth::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_speed"), &GDSAMSynth::set_speed);
    ClassDB::bind_method(D_METHOD("set_pitch"), &GDSAMSynth::set_pitch);
    ClassDB::bind_method(D_METHOD("set_mouth"), &GDSAMSynth::set_mouth);
    ClassDB::bind_method(D_METHOD("set_throat"), &GDSAMSynth::set_throat);
    ClassDB::bind_method(D_METHOD("set_singing"), &GDSAMSynth::set_singing);
    ClassDB::bind_method(D_METHOD("set_phonetic"), &GDSAMSynth::set_phonetic);
    ClassDB::bind_method(D_METHOD("get_last_error"), &GDSAMSynth::get_last_error);
    ClassDB::bind_method(D_METHOD("speak"), &GDSAMSynth::speak);
}

