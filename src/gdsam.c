#include <gdnative_api_struct.gen.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"
#include "godot_util.h"

int debug = 0;

typedef struct user_data_struct {
    int64_t speed;
    int64_t pitch;
    int64_t mouth;
    int64_t throat;
    bool singing;
    bool phonetic;
} user_data_struct;

const godot_gdnative_core_api_struct *api = NULL;
const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

void *gdsam_constructor(godot_object *p_instance, void *p_method_data);
void gdsam_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data);
godot_variant gdsam_set_speed(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_set_pitch(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_set_mouth(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_set_throat(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_set_singing(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_set_phonetic(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant gdsam_speak(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);

void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *p_options) {
    api = p_options->api_struct;

    for (size_t i = 0; i < api->num_extensions; i++) {
        switch (api->extensions[i]->type) {
            case GDNATIVE_EXT_NATIVESCRIPT: {
                nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
            }; break;
            default: break;
        }
    }    
}

void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *p_options) {
    api = NULL;
    nativescript_api = NULL;
}

void GDN_EXPORT godot_nativescript_init(void *p_handle) {
    godot_instance_create_func create = { NULL, NULL, NULL };
    create.create_func = &gdsam_constructor;

    godot_instance_destroy_func destroy = { NULL, NULL, NULL };
    destroy.destroy_func = &gdsam_destructor;

    nativescript_api->godot_nativescript_register_class(p_handle, "GDSAM", "Reference", create, destroy);

    godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

    GD_REGISTER_METHOD("GDSAM", set_speed, gdsam_set_speed);
    GD_REGISTER_METHOD("GDSAM", set_pitch, gdsam_set_pitch);
    GD_REGISTER_METHOD("GDSAM", set_mouth, gdsam_set_mouth);
    GD_REGISTER_METHOD("GDSAM", set_throat, gdsam_set_throat);
    GD_REGISTER_METHOD("GDSAM", set_singing, gdsam_set_singing);
    GD_REGISTER_METHOD("GDSAM", set_phonetic, gdsam_set_phonetic);
    GD_REGISTER_METHOD("GDSAM", speak, gdsam_speak);
}

void *gdsam_constructor(godot_object *p_instance, void *p_method_data) {
    user_data_struct *user_data = api->godot_alloc(sizeof(user_data_struct));
    user_data->speed = 64;
    user_data->pitch = 64;
    user_data->mouth = 128;
    user_data->throat = 128;
    user_data->singing = false;

    return user_data;
}

void gdsam_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data) {
    api->godot_free(p_user_data);
}

godot_variant gdsam_set_speed(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *speed_var = p_args[0];
    user_data->speed = GD_CAST_VARIANT(int, speed_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_set_pitch(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *pitch_var = p_args[0];
    user_data->pitch = GD_CAST_VARIANT(int, pitch_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_set_mouth(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *mouth_var = p_args[0];
    user_data->mouth = GD_CAST_VARIANT(int, mouth_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_set_throat(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *throat_var = p_args[0];
    user_data->throat = GD_CAST_VARIANT(int, throat_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_set_singing(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *singing_var = p_args[0];
    user_data->singing = GD_CAST_VARIANT(bool, singing_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_set_phonetic(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();

    godot_variant *phonetic_var = p_args[0];
    user_data->phonetic = GD_CAST_VARIANT(bool, phonetic_var);

    GD_RETURN_NULL();
}

godot_variant gdsam_speak(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
    if(p_num_args < 1) {
        GD_RETURN_NULL();
    }

    GD_USER_DATA();
    
    godot_variant *input_var = p_args[0];
    godot_string input_str = api->godot_variant_as_string(input_var);
    godot_char_string input_char_str = api->godot_string_utf8(&input_str);
    api->godot_string_destroy(&input_str);
    const char *input = api->godot_char_string_get_data(&input_char_str);
    api->godot_char_string_destroy(&input_char_str);

    SetSpeed(user_data->speed & 0xFF);
    SetPitch(user_data->pitch & 0xFF);
    SetMouth(user_data->mouth & 0xFF);
    SetThroat(user_data->throat & 0xFF);
    if(user_data->singing) {
        EnableSingmode();
    }

    char _input[256];
    for(int i = 0; i < 256; i++) _input[i] = 0;
    strcpy(_input, input);
    for(int i = 0; _input[i] != 0; i++) _input[i] = toupper((int)_input[i]);

    if(!user_data->phonetic) {
        strncat(_input, "[", 255);
        if(!TextToPhonemes((unsigned char *)_input)) {
            GD_RETURN_NULL();
        }
    } else {
        strncat(_input, "\x9b", 255);
    }

    SetInput(_input);
    if (!SAMMain()) {
        GD_RETURN_NULL();
    }

    int buffer_length = GetBufferLength() / 50;
    char *buffer = GetBuffer();

    godot_pool_byte_array g_buffer;
    api->godot_pool_byte_array_new(&g_buffer);

    for(int i = 0; i < buffer_length; i++) {
        uint8_t data = buffer[i] - 128;
        api->godot_pool_byte_array_append(&g_buffer, data);
    }

    godot_variant g_buffer_var;
    api->godot_variant_new_pool_byte_array(&g_buffer_var, &g_buffer);
    api->godot_pool_byte_array_destroy(&g_buffer);

    return g_buffer_var;
}