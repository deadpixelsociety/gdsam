#ifndef RECITER_C
#define RECITER_C

#include <stddef.h>

/* On success, the caller owns the NUL-terminated output and releases it with free. */
int ReciteText(const char *text, size_t length, char **phonemes, size_t *phoneme_length);

int TextToPhonemes(unsigned char *input);

#endif
