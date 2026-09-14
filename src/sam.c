#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "sam.h"
#include "render.h"
#include "SamTabs.h"
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

static char *input;
static size_t input_length;
//standard sam sound
unsigned char speed = 72;
unsigned char pitch = 64;
unsigned char mouth = 128;
unsigned char throat = 128;
int singmode = 0;

extern int debug;

unsigned char mem39;
unsigned char mem44;
unsigned char mem47;
unsigned char mem49;
unsigned char mem50;
unsigned char mem51;
unsigned char mem53;
unsigned char mem56;

unsigned char mem59=0;

unsigned char A1, Y1;
static ptrdiff_t X1;

unsigned char *stress, *phonemeLength, *phonemeindex;
size_t phoneme_count;
static size_t phoneme_capacity, length_capacity, stress_capacity;
static size_t buffer_capacity;
static char sam_error[192];
#ifdef SAM_TESTING
size_t sam_render_segments, sam_render_phonemes;
static ptrdiff_t allocation_countdown = -1;
void SamFailAllocationAfter(ptrdiff_t count) { allocation_countdown = count; }
#endif

void SamSetError(const char *format, ...) {
    va_list args;
    if (sam_error[0]) return;
    va_start(args, format);
    vsnprintf(sam_error, sizeof(sam_error), format, args);
    va_end(args);
}
const char *SamGetError(void) { return sam_error; }

static void *SamRealloc(void *data, size_t size) {
    void *result;
#ifdef SAM_TESTING
    if (allocation_countdown == 0) { SamSetError("Out of memory"); return NULL; }
    if (allocation_countdown > 0) --allocation_countdown;
#endif
    result = realloc(data, size);
    if (!result) SamSetError("Out of memory");
    return result;
}
void *SamAlloc(size_t size) { return SamRealloc(NULL, size); }
void *SamGrow(void *data, size_t *capacity, size_t needed) {
    size_t next = *capacity ? *capacity : 256;
    void *result;
    if (needed <= *capacity) return data;
    if (needed > PTRDIFF_MAX) { SamSetError("Buffer size exceeds supported range"); return 0; }
    while (next < needed) {
        if (next > (size_t)PTRDIFF_MAX / 2) { next = needed; break; }
        next *= 2;
    }
    result = SamRealloc(data, next);
    if (!result) return 0;
    *capacity = next;
    return result;
}
static int ReservePhonemes(size_t count) {
    if (count > (size_t)PTRDIFF_MAX - 1) { SamSetError("Too many phonemes"); return 0; }
    unsigned char *next = SamGrow(phonemeindex, &phoneme_capacity, count + 1);
    if (!next) return 0;
    phonemeindex = next;
    next = SamGrow(phonemeLength, &length_capacity, count + 1);
    if (!next) return 0;
    phonemeLength = next;
    next = SamGrow(stress, &stress_capacity, count + 1);
    if (!next) return 0;
    stress = next;
    return 1;
}
static unsigned char PhonemeAt(ptrdiff_t position) {
    return position < 0 || (size_t)position >= phoneme_count ? 255 : phonemeindex[position];
}
static unsigned char Flags(unsigned int id) { return id < sizeof(flags) ? flags[id] : 0; }
static unsigned char Flags2(unsigned int id) { return id < sizeof(flags2) ? flags2[id] : 0; }


unsigned char phonemeIndexOutput[60]; //tab47296
unsigned char stressOutput[60]; //tab47365
unsigned char phonemeLengthOutput[60]; //tab47416




// contains the final soundbuffer
uint64_t bufferpos=0;
char *buffer = NULL;

void FreeSam(void) {
    free(input); input = NULL; input_length = 0;
    free(phonemeindex); phonemeindex = NULL;
    free(phonemeLength); phonemeLength = NULL;
    free(stress); stress = NULL;
    free(buffer); buffer = NULL;
    phoneme_count = phoneme_capacity = length_capacity = stress_capacity = 0;
    buffer_capacity = 0;
    bufferpos = 0;
}

void ResetSam(void) {
    FreeSam();
    sam_error[0] = 0;
    memset(phonemeIndexOutput, 0, sizeof(phonemeIndexOutput));
    memset(phonemeLengthOutput, 0, sizeof(phonemeLengthOutput));
    memset(stressOutput, 0, sizeof(stressOutput));
    speed = 72; pitch = 64; mouth = throat = 128; singmode = debug = 0;
    mem39 = mem44 = mem47 = mem49 = mem50 = mem51 = mem53 = mem56 = mem59 = 0;
    A1 = Y1 = 0; X1 = 0;
    ResetRender();
}

int SetInputText(const char *text, size_t length) {
    char *next;
    if (length > PTRDIFF_MAX - 2) { SamSetError("Phonetic input is too large"); return 0; }
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126 || c == '[' || c == ']') {
            SamSetError("Unsupported phonetic byte at position %zu", i); return 0;
        }
    }
    next = SamAlloc(length + 2);
    if (!next) return 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        next[i] = c >= 'a' && c <= 'z' ? c - 32 : c;
    }
    next[length] = (char)155; next[length + 1] = 0;
    free(input); input = next; input_length = length;
    return 1;
}
void SetInput(char *text) {
    size_t length = 0;
    while (text[length] && (unsigned char)text[length] != 155) ++length;
    SetInputText(text, length);
}

int SamWriteAudio(int advance, const unsigned char *samples, size_t count) {
    size_t position, previous = buffer_capacity;
    if (sam_error[0]) return 0;
    if (advance < 0 || bufferpos > UINT64_MAX - (unsigned)advance) {
        SamSetError("Audio timing exceeds supported range"); return 0;
    }
    bufferpos += (unsigned)advance;
    if (count > (size_t)PTRDIFF_MAX || bufferpos / 50 > (uint64_t)PTRDIFF_MAX - count) {
        SamSetError("Audio exceeds supported buffer size"); return 0;
    }
    position = (size_t)(bufferpos / 50);
    char *next = SamGrow(buffer, &buffer_capacity, position + count);
    if (!next) return 0;
    buffer = next;
    if (buffer_capacity > previous) memset(buffer + previous, 128, buffer_capacity - previous);
    memcpy(buffer + position, samples, count);
    return 1;
}

void SetSpeed(unsigned char _speed) {speed = _speed;}
void SetPitch(unsigned char _pitch) {pitch = _pitch;}
void SetMouth(unsigned char _mouth) {mouth = _mouth;}
void SetThroat(unsigned char _throat) {throat = _throat;}
void EnableSingmode() {singmode = 1;}
char* GetBuffer(){return buffer;}
uint64_t GetBufferLength(){return bufferpos;}

void Init();
int Parser1();
void Parser2();
int SAMMain();
void CopyStress();
void SetPhonemeLength();
void AdjustLengths();
void Code41240();
int Insert(ptrdiff_t position, unsigned char mem60, unsigned char mem59, unsigned char mem58);
void PrepareOutput();
void SetMouthThroat(unsigned char mouth, unsigned char throat);

// 168=pitches
// 169=frequency1
// 170=frequency2
// 171=frequency3
// 172=amplitude1
// 173=amplitude2
// 174=amplitude3


void Init(void) {
    SetMouthThroat(mouth, throat);
    bufferpos = 0;
    phoneme_count = 0;
}

int SAMMain(void) {
    if (sam_error[0]) return 0;
    if (!input) { SamSetError("No phonetic input"); return 0; }
    Init();
    if (!Parser1()) return 0;
    Parser2();
    if (sam_error[0]) return 0;
    CopyStress();
    SetPhonemeLength();
    if (sam_error[0]) return 0;
    AdjustLengths();
    Code41240();
    if (sam_error[0]) return 0;
    PrepareOutput();
    return !sam_error[0];
}

/* Keep the legacy renderer bounded while consuming the entire phoneme list. */
void PrepareOutput(void) {
    size_t start = 0;
#ifdef SAM_TESTING
    sam_render_segments = sam_render_phonemes = 0;
#endif
    while (start < phoneme_count && !sam_error[0]) {
        size_t end = start, boundary = start, entries = 0, frames = 0;
        while (end < phoneme_count) {
            unsigned char id = phonemeindex[end];
            size_t length = phonemeLength[end];
            if (id == 0) { boundary = ++end; continue; }
            if (id >= 78 || !length || length >= 232) {
                SamSetError("Invalid render phoneme at position %zu", end); return;
            }
            if (entries == 59 || frames + length >= 232) break;
            ++entries; frames += length; ++end;
            if (Flags2(id) & 1) { boundary = end; break; }
        }
        if (end < phoneme_count && boundary > start) end = boundary;
        if (end == start) { SamSetError("Unable to advance render segment"); return; }
        entries = 0;
        memset(phonemeIndexOutput, 255, sizeof(phonemeIndexOutput));
        memset(phonemeLengthOutput, 0, sizeof(phonemeLengthOutput));
        memset(stressOutput, 0, sizeof(stressOutput));
        for (size_t i = start; i < end; ++i) {
            if (!phonemeindex[i]) continue;
            phonemeIndexOutput[entries] = phonemeindex[i];
            phonemeLengthOutput[entries] = phonemeLength[i];
            stressOutput[entries++] = stress[i];
        }
        Render();
#ifdef SAM_TESTING
        if (entries) ++sam_render_segments;
        sam_render_phonemes += entries;
#endif
        start = end;
    }
}

// Iterates through the phoneme buffer, copying the stress value from
// the following phoneme under the following circumstance:

//     1. The current phoneme is voiced, excluding plosives and fricatives
//     2. The following phoneme is voiced, excluding plosives and fricatives, and
//     3. The following phoneme is stressed
//
//  In those cases, the stress value+1 from the following phoneme is copied.
//
// For example, the word LOITER is represented as LOY5TER, with as stress
// of 5 on the diphtong OY. This routine will copy the stress value of 6 (5+1)
// to the L that precedes it.


//void Code41883()
void CopyStress()
{
    // loop thought all the phonemes to be output
    ptrdiff_t pos=0; //mem66
    while(1)
    {
        // get the phomene
        Y1 = PhonemeAt(pos);

        // exit at end of buffer
        if (Y1 == 255) return;

        // if CONSONANT_FLAG set, skip - only vowels get stress
        if ((Flags(Y1) & 64) == 0) {pos++; continue;}
        // get the next phoneme
        Y1 = PhonemeAt(pos+1);
        if (Y1 == 255) //prevent buffer overflow
        {
            pos++; continue;
        } else
        // if the following phoneme is a vowel, skip
        if ((Flags(Y1) & 128) == 0)  {pos++; continue;}

        // get the stress value at the next position
        Y1 = stress[pos+1];

        // if next phoneme is not stressed, skip
        if (Y1 == 0)  {pos++; continue;}

        // if next phoneme is not a VOWEL OR ER, skip
        if ((Y1 & 128) != 0)  {pos++; continue;}

        // copy stress from prior phoneme to this one
        stress[pos] = Y1+1;

        // advance pointer
        pos++;
    }

}


//void Code41014()
int Insert(ptrdiff_t position, unsigned char id, unsigned char length, unsigned char value) {
    size_t tail;
    if (position < 0 || (size_t)position > phoneme_count) {
        SamSetError("Invalid phoneme insertion position"); return 0;
    }
    if (!ReservePhonemes(phoneme_count + 1)) return 0;
    tail = phoneme_count - (size_t)position;
    memmove(phonemeindex + position + 1, phonemeindex + position, tail);
    memmove(phonemeLength + position + 1, phonemeLength + position, tail);
    memmove(stress + position + 1, stress + position, tail);
    phonemeindex[position] = id;
    phonemeLength[position] = length;
    stress[position] = value;
    ++phoneme_count;
    phonemeindex[phoneme_count] = 255;
    phonemeLength[phoneme_count] = stress[phoneme_count] = 0;
    return 1;
}

// The input[] buffer contains a string of phonemes and stress markers along
// the lines of:
//
//     DHAX KAET IHZ AH5GLIY. <0x9B>
//
// The byte 0x9B marks the end of the buffer. Some phonemes are 2 bytes
// long, such as "DH" and "AX". Others are 1 byte long, such as "T" and "Z".
// There are also stress markers, such as "5" and ".".
//
// The first character of the phonemes are stored in the table signInputTable1[].
// The second character of the phonemes are stored in the table signInputTable2[].
// The stress characters are arranged in low to high stress order in stressInputTable[].
//
// The following process is used to parse the input[] buffer:
//
// Repeat until the <0x9B> character is reached:
//
//        First, a search is made for a 2 character match for phonemes that do not
//        end with the '*' (wildcard) character. On a match, the index of the phoneme
//        is added to phonemeIndex[] and the buffer position is advanced 2 bytes.
//
//        If this fails, a search is made for a 1 character match against all
//        phoneme names ending with a '*' (wildcard). If this succeeds, the
//        phoneme is added to phonemeIndex[] and the buffer position is advanced
//        1 byte.
//
//        If this fails, search for a 1 character match in the stressInputTable[].
//        If this succeeds, the stress value is placed in the last stress[] table
//        at the same index of the last added phoneme, and the buffer position is
//        advanced by 1 byte.
//
//        If this fails, return a 0.
//
// On success:
//
//    1. phonemeIndex[] will contain the index of all the phonemes.
//    2. The last index in phonemeIndex[] will be 255.
//    3. stress[] will contain the stress value for each phoneme

// input[] holds the string of phonemes, each two bytes wide
// signInputTable1[] holds the first character of each phoneme
// signInputTable2[] holds te second character of each phoneme
// phonemeIndex[] holds the indexes of the phonemes after parsing input[]
//
// The parser scans through the input[], finding the names of the phonemes
// by searching signInputTable1[] and signInputTable2[]. On a match, it
// copies the index of the phoneme into the phonemeIndexTable[].
//
// The character <0x9B> marks the end of text in input[]. When it is reached,
// the index 255 is placed at the end of the phonemeIndexTable[], and the
// function returns with a 1 indicating success.
int Parser1(void) {
    size_t position = 0;
    if (!ReservePhonemes(0)) return 0;
    phonemeindex[0] = 255;
    while (position < input_length) {
        unsigned char first = (unsigned char)input[position];
        unsigned char second = position + 1 < input_length ? (unsigned char)input[position + 1] : 0;
        size_t id;
        for (id = 0; id < sizeof(signInputTable1); ++id) {
            if (signInputTable2[id] != '*' && signInputTable1[id] == first && signInputTable2[id] == second) break;
        }
        if (id < sizeof(signInputTable1)) {
            if (!Insert((ptrdiff_t)phoneme_count, (unsigned char)id, 0, 0)) return 0;
            position += 2; continue;
        }
        for (id = 0; id < sizeof(signInputTable1); ++id) {
            if (signInputTable1[id] != '*' && signInputTable2[id] == '*' && signInputTable1[id] == first) break;
        }
        if (id < sizeof(signInputTable1)) {
            if (!Insert((ptrdiff_t)phoneme_count, (unsigned char)id, 0, 0)) return 0;
            ++position; continue;
        }
        if (first >= '1' && first <= '8' && phoneme_count && phonemeindex[phoneme_count - 1] > 4) {
            stress[phoneme_count - 1] = first - '0'; ++position; continue;
        }
        SamSetError("Invalid phonetic token or stress at position %zu", position); return 0;
    }
    return 1;
}

//change phonemelength depedendent on stress
//void Code41203()
void SetPhonemeLength()
{
    unsigned char A1;
    ptrdiff_t position = 0;
    while(PhonemeAt(position) != 255 )
    {
        if (PhonemeAt(position) >= sizeof(phonemeLengthTable)) { SamSetError("Unexpanded phoneme"); return; }
        A1 = stress[position];
        //41218: BMI 41229
        if ((A1 == 0) || ((A1&128) != 0))
        {
            phonemeLength[position] = phonemeLengthTable[PhonemeAt(position)];
        } else
        {
            phonemeLength[position] = phonemeStressedLengthTable[PhonemeAt(position)];
        }
        position++;
    }
}


void Code41240()
{
    ptrdiff_t pos=0;

    while(PhonemeAt(pos) != 255)
    {
        unsigned char index; //register AC
        X1 = pos;
        index = PhonemeAt(pos);
        if ((Flags(index)&2) == 0)
        {
            pos++;
            continue;
        } else
        if ((Flags(index)&1) == 0)
        {
            if (!Insert(pos+1, index+1, phonemeLengthTable[index+1], stress[pos])) return;
            if (!Insert(pos+2, index+2, phonemeLengthTable[index+2], stress[pos])) return;
            pos += 3;
            continue;
        }

        do
        {
            X1++;
            A1 = PhonemeAt(X1);
        } while(A1==0);

        if (A1 != 255)
        {
            if ((Flags(A1) & 8) != 0)  {pos++; continue;}
            if ((A1 == 36) || (A1 == 37)) {pos++; continue;} // '/H' '/X'
        }

        if (!Insert(pos+1, index+1, phonemeLengthTable[index+1], stress[pos])) return;
        if (!Insert(pos+2, index+2, phonemeLengthTable[index+2], stress[pos])) return;
        pos += 3;
    };

}

// Rewrites the phonemes using the following rules:
//
//       <DIPHTONG ENDING WITH WX> -> <DIPHTONG ENDING WITH WX> WX
//       <DIPHTONG NOT ENDING WITH WX> -> <DIPHTONG NOT ENDING WITH WX> YX
//       UL -> AX L
//       UM -> AX M
//       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL> <SILENCE> Q <VOWEL>
//       T R -> CH R
//       D R -> J R
//       <VOWEL> R -> <VOWEL> RX
//       <VOWEL> L -> <VOWEL> LX
//       G S -> G Z
//       K <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPHTONG NOT ENDING WITH IY>
//       G <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPHTONG NOT ENDING WITH IY>
//       S P -> S B
//       S T -> S D
//       S K -> S G
//       S KX -> S GX
//       <ALVEOLAR> UW -> <ALVEOLAR> UX
//       CH -> CH CH' (CH requires two phonemes to represent it)
//       J -> J J' (J requires two phonemes to represent it)
//       <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX <PAUSE>
//       <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>


//void Code41397()
void Parser2()
{
    if (debug) printf("Parser2\n");
    ptrdiff_t pos = 0; //mem66;
    unsigned char mem58 = 0;


  // Loop through phonemes
    while(1)
    {
// SET X TO THE CURRENT POSITION
        X1 = pos;
// GET THE PHONEME AT THE CURRENT POSITION
        A1 = PhonemeAt(pos);

// DEBUG: Print phoneme and index
        if (debug && A1 != 255) printf("%td: %c%c\n", X1, signInputTable1[A1], signInputTable2[A1]);

// Is phoneme pause?
        if (A1 == 0)
        {
// Move ahead to the
            pos++;
            continue;
        }

// If end of phonemes flag reached, exit routine
        if (A1 == 255) return;

// Copy the current phoneme index to Y
        Y1 = A1;

// RULE:
//       <DIPHTONG ENDING WITH WX> -> <DIPHTONG ENDING WITH WX> WX
//       <DIPHTONG NOT ENDING WITH WX> -> <DIPHTONG NOT ENDING WITH WX> YX
// Example: OIL, COW


// Check for DIPHTONG
        if ((Flags(A1) & 16) == 0) goto pos41457;

// Not a diphthong. Get the stress
        mem58 = stress[pos];

// End in IY sound?
        A1 = Flags(Y1) & 32;

// If ends with IY, use YX, else use WX
        if (A1 == 0) A1 = 20; else A1 = 21;    // 'WX' = 20 'YX' = 21
        //pos41443:
// Insert at WX or YX following, copying the stress

        if (debug) if (A1==20) printf("RULE: insert WX following diphtong NOT ending in IY sound\n");
        if (debug) if (A1==21) printf("RULE: insert YX following diphtong ending in IY sound\n");
        if (!Insert(pos+1, A1, mem59, mem58)) return;
        X1 = pos;
// Jump to ???
        goto pos41749;



pos41457:

// RULE:
//       UL -> AX L
// Example: MEDDLE

// Get phoneme
        A1 = PhonemeAt(X1);
// Skip this rule if phoneme is not UL
        if (A1 != 78) goto pos41487;  // 'UL'
        A1 = 24;         // 'L'                 //change 'UL' to 'AX L'

        if (debug) printf("RULE: UL -> AX L\n");

pos41466:
// Get current phoneme stress
        mem58 = stress[X1];

// Change UL to AX
        phonemeindex[X1] = 13;  // 'AX'
// Perform insert. Note code below may jump up here with different values
        if (!Insert(X1+1, A1, mem59, mem58)) return;
        pos++;
// Move to next phoneme
        continue;

pos41487:

// RULE:
//       UM -> AX M
// Example: ASTRONOMY

// Skip rule if phoneme != UM
        if (A1 != 79) goto pos41495;   // 'UM'
        // Jump up to branch - replaces current phoneme with AX and continues
        A1 = 27; // 'M'  //change 'UM' to  'AX M'
        if (debug) printf("RULE: UM -> AX M\n");
        goto pos41466;
pos41495:

// RULE:
//       UN -> AX N
// Example: FUNCTION


// Skip rule if phoneme != UN
        if (A1 != 80) goto pos41503; // 'UN'

        // Jump up to branch - replaces current phoneme with AX and continues
        A1 = 28;         // 'N' //change UN to 'AX N'
        if (debug) printf("RULE: UN -> AX N\n");
        goto pos41466;
pos41503:

// RULE:
//       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL> <SILENCE> Q <VOWEL>
// EXAMPLE: AWAY EIGHT

        Y1 = A1;
// VOWEL set?
        A1 = Flags(A1) & 128;

// Skip if not a vowel
        if (A1 != 0)
        {
// Get the stress
            A1 = stress[X1];

// If stressed...
            if (A1 != 0)
            {
// Get the following phoneme
                X1++;
                A1 = PhonemeAt(X1);
// If following phoneme is a pause

                if (A1 == 0)
                {
// Get the phoneme following pause
                    X1++;
                    Y1 = PhonemeAt(X1);

// Check for end of buffer flag
                    if (Y1 == 255) //buffer overflow
// ??? Not sure about these flags
                        A1 = 65&128;
                    else
// And VOWEL flag to current phoneme's flags
                        A1 = Flags(Y1) & 128;

// If following phonemes is not a pause
                    if (A1 != 0)
                    {
// If the following phoneme is not stressed
                        A1 = stress[X1];
                        if (A1 != 0)
                        {
// Insert a glottal stop and move forward
                            if (debug) printf("RULE: Insert glottal stop between two stressed vowels with space between them\n");
                            // 31 = 'Q'
                            if (!Insert(X1, 31, mem59, 0)) return;
                            pos++;
                            continue;
                        }
                    }
                }
            }
        }


// RULES FOR PHONEMES BEFORE R
//        T R -> CH R
// Example: TRACK


// Get current position and phoneme
        X1 = pos;
        A1 = PhonemeAt(pos);
        if (A1 != 23) goto pos41611;     // 'R'

// Look at prior phoneme
        X1--;
        A1 = PhonemeAt(pos-1);
        //pos41567:
        if (A1 == 69)                    // 'T'
        {
// Change T to CH
            if (debug) printf("RULE: T R -> CH R\n");
            phonemeindex[pos-1] = 42;
            goto pos41779;
        }


// RULES FOR PHONEMES BEFORE R
//        D R -> J R
// Example: DRY

// Prior phonemes D?
        if (A1 == 57)                    // 'D'
        {
// Change D to J
            phonemeindex[pos-1] = 44;
            if (debug) printf("RULE: D R -> J R\n");
            goto pos41788;
        }

// RULES FOR PHONEMES BEFORE R
//        <VOWEL> R -> <VOWEL> RX
// Example: ART


// If vowel flag is set change R to RX
        A1 = Flags(A1) & 128;
        if (debug) printf("RULE: R -> RX\n");
        if (A1 != 0) phonemeindex[pos] = 18;  // 'RX'

// continue to next phoneme
        pos++;
        continue;

pos41611:

// RULE:
//       <VOWEL> L -> <VOWEL> LX
// Example: ALL

// Is phoneme L?
        if (A1 == 24)    // 'L'
        {
// If prior phoneme does not have VOWEL flag set, move to next phoneme
            if ((Flags(PhonemeAt(pos-1)) & 128) == 0) {pos++; continue;}
// Prior phoneme has VOWEL flag set, so change L to LX and move to next phoneme
            if (debug) printf("RULE: <VOWEL> L -> <VOWEL> LX\n");
            phonemeindex[X1] = 19;     // 'LX'
            pos++;
            continue;
        }

// RULE:
//       G S -> G Z
//
// Can't get to fire -
//       1. The G -> GX rule intervenes
//       2. Reciter already replaces GS -> GZ

// Is current phoneme S?
        if (A1 == 32)    // 'S'
        {
// If prior phoneme is not G, move to next phoneme
            if (PhonemeAt(pos-1) != 60) {pos++; continue;}
// Replace S with Z and move on
            if (debug) printf("RULE: G S -> G Z\n");
            phonemeindex[pos] = 38;    // 'Z'
            pos++;
            continue;
        }

// RULE:
//             K <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPHTONG NOT ENDING WITH IY>
// Example: COW

// Is current phoneme K?
        if (A1 == 72)    // 'K'
        {
// Get next phoneme
            Y1 = PhonemeAt(pos+1);
// If at end, replace current phoneme with KX
            if (Y1 == 255) phonemeindex[pos] = 75; // ML : prevents an index out of bounds problem
            else
            {
// VOWELS AND DIPHTONGS ENDING WITH IY SOUND flag set?
                A1 = Flags(Y1) & 32;
                if (debug) if (A1==0) printf("RULE: K <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPHTONG NOT ENDING WITH IY>\n");
// Replace with KX
                if (A1 == 0) phonemeindex[pos] = 75;  // 'KX'
            }
        }
        else

// RULE:
//             G <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPHTONG NOT ENDING WITH IY>
// Example: GO


// Is character a G?
        if (A1 == 60)   // 'G'
        {
// Get the following character
            unsigned char index = PhonemeAt(pos+1);

// At end of buffer?
            if (index == 255) //prevent buffer overflow
            {
                pos++; continue;
            }
            else
// If diphtong ending with YX, move continue processing next phoneme
            if ((Flags(index) & 32) != 0) {pos++; continue;}
// replace G with GX and continue processing next phoneme
            if (debug) printf("RULE: G <VOWEL OR DIPHTONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPHTONG NOT ENDING WITH IY>\n");
            phonemeindex[pos] = 63; // 'GX'
            pos++;
            continue;
        }

// RULE:
//      S P -> S B
//      S T -> S D
//      S K -> S G
//      S KX -> S GX
// Examples: SPY, STY, SKY, SCOWL

        Y1 = PhonemeAt(pos);
        //pos41719:
// Replace with softer version?
        A1 = Flags(Y1) & 1;
        if (A1 == 0) goto pos41749;
        A1 = PhonemeAt(pos-1);
        if (A1 != 32)    // 'S'
        {
            A1 = Y1;
            goto pos41812;
        }
        // Replace with softer version
        if (debug) printf("RULE: S* %c%c -> S* %c%c\n", signInputTable1[Y1], signInputTable2[Y1],signInputTable1[Y1-12], signInputTable2[Y1-12]);
        phonemeindex[pos] = Y1-12;
        pos++;
        continue;


pos41749:

// RULE:
//      <ALVEOLAR> UW -> <ALVEOLAR> UX
//
// Example: NEW, DEW, SUE, ZOO, THOO, TOO

//       UW -> UX

        A1 = PhonemeAt(X1);
        if (A1 == 53)    // 'UW'
        {
// ALVEOLAR flag set?
            Y1 = PhonemeAt(X1-1);
            A1 = Flags2(Y1) & 4;
// If not set, continue processing next phoneme
            if (A1 == 0) {pos++; continue;}
            if (debug) printf("RULE: <ALVEOLAR> UW -> <ALVEOLAR> UX\n");
            phonemeindex[X1] = 16;
            pos++;
            continue;
        }
pos41779:

// RULE:
//       CH -> CH CH' (CH requires two phonemes to represent it)
// Example: CHEW

        if (A1 == 42)    // 'CH'
        {
            //        pos41783:
            if (debug) printf("CH -> CH CH+1\n");
            if (!Insert(X1+1, A1+1, mem59, stress[X1])) return;
            pos++;
            continue;
        }

pos41788:

// RULE:
//       J -> J J' (J requires two phonemes to represent it)
// Example: JAY


        if (A1 == 44) // 'J'
        {
            if (debug) printf("J -> J J+1\n");
            if (!Insert(X1+1, A1+1, mem59, stress[X1])) return;
            pos++;
            continue;
        }

// Jump here to continue
pos41812:

// RULE: Soften T following vowel
// NOTE: This rule fails for cases such as "ODD"
//       <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX <PAUSE>
//       <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>
// Example: PARTY, TARDY


// Past this point, only process if phoneme is T or D

        if (A1 != 69)    // 'T'
        if (A1 != 57) {pos++; continue;}       // 'D'
        //pos41825:


// If prior phoneme is not a vowel, continue processing phonemes
        if ((Flags(PhonemeAt(X1-1)) & 128) == 0) {pos++; continue;}

// Get next phoneme
        X1++;
        A1 = PhonemeAt(X1);
        //pos41841
// Is the next phoneme a pause?
        if (A1 != 0)
        {
// If next phoneme is not a pause, continue processing phonemes
            if ((Flags(A1) & 128) == 0) {pos++; continue;}
// If next phoneme is stressed, continue processing phonemes
// FIXME: How does a pause get stressed?
            if (stress[X1] != 0) {pos++; continue;}
//pos41856:
// Set phonemes to DX
        if (debug) printf("RULE: Soften T or D following vowel or ER and preceding a pause -> DX\n");
        phonemeindex[pos] = 30;       // 'DX'
        } else
        {
            A1 = PhonemeAt(X1+1);
            if (A1 == 255) //prevent buffer overflow
                A1 = 65 & 128;
            else
// Is next phoneme a vowel or ER?
                A1 = Flags(A1) & 128;
            if (debug) if (A1 != 0) printf("RULE: Soften T or D following vowel or ER and preceding a pause -> DX\n");
            if (A1 != 0) phonemeindex[pos] = 30;  // 'DX'
        }

        pos++;

    } // while
}


// Applies various rules that adjust the lengths of phonemes
//
//         Lengthen <FRICATIVE> or <VOICED> between <VOWEL> and <PUNCTUATION> by 1.5
//         <VOWEL> <RX | LX> <CONSONANT> - decrease <VOWEL> length by 1
//         <VOWEL> <UNVOICED PLOSIVE> - decrease vowel by 1/8th
//         <VOWEL> <UNVOICED CONSONANT> - increase vowel by 1/2 + 1
//         <NASAL> <STOP CONSONANT> - set nasal = 5, consonant = 6
//         <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT> - shorten both to 1/2 + 1
//         <LIQUID CONSONANT> <DIPHTONG> - decrease by 2


//void Code48619()
void AdjustLengths()
{

    // LENGTHEN VOWELS PRECEDING PUNCTUATION
    //
    // Search for punctuation. If found, back up to the first vowel, then
    // process all phonemes between there and up to (but not including) the punctuation.
    // If any phoneme is found that is a either a fricative or voiced, the duration is
    // increased by (length * 1.5) + 1

    // loop index
    X1 = 0;
    unsigned char index;

    // iterate through the phoneme list
    ptrdiff_t loopIndex=0;
    while(1)
    {
        // get a phoneme
        index = PhonemeAt(X1);

        // exit loop if end on buffer token
        if (index == 255) break;

        // not punctuation?
        if((Flags2(index) & 1) == 0)
        {
            // skip
            X1++;
            continue;
        }

        // hold index
        loopIndex = X1;

        // Loop backwards from this point
pos48644:

        // back up one phoneme
        if (X1 == 0) break;
        X1--;

        // stop once the beginning is reached
        if(X1 == 0) break;

        // get the preceding phoneme
        index = PhonemeAt(X1);

        if (index != 255) //inserted to prevent access overrun
        if((Flags(index) & 128) == 0) goto pos48644; // if not a vowel, continue looping

        //pos48657:
        do
        {
            // test for vowel
            index = PhonemeAt(X1);

            if (index != 255)//inserted to prevent access overrun
            // test for fricative/unvoiced or not voiced
            if(((Flags2(index) & 32) == 0) || ((Flags(index) & 4) != 0))     //nochmal �berpr�fen
            {
                //A = Flags(Y) & 4;
                //if(A == 0) goto pos48688;

                // get the phoneme length
                A1 = phonemeLength[X1];

                // change phoneme length to (length * 1.5) + 1
                A1 = (A1 >> 1) + A1 + 1;
if (debug) printf("RULE: Lengthen <FRICATIVE> or <VOICED> between <VOWEL> and <PUNCTUATION> by 1.5\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);

                phonemeLength[X1] = A1;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);

            }
            // keep moving forward
            X1++;
        } while (X1 != loopIndex);
        //  if (X != loopIndex) goto pos48657;
        X1++;
    }  // while

    // Similar to the above routine, but shorten vowels under some circumstances

    // Loop throught all phonemes
    loopIndex = 0;
    //pos48697

    while(1)
    {
        // get a phoneme
        X1 = loopIndex;
        index = PhonemeAt(X1);

        // exit routine at end token
        if (index == 255) return;

        // vowel?
        A1 = Flags(index) & 128;
        if (A1 != 0)
        {
            // get next phoneme
            X1++;
            index = PhonemeAt(X1);

            // get flags
            if (index == 255)
            mem56 = 65; // use if end marker
            else
            mem56 = Flags(index);

            // not a consonant
            if ((mem56 & 64) == 0)
            {
                // RX or LX?
                if ((index == 18) || (index == 19))  // 'RX' & 'LX'
                {
                    // get the next phoneme
                    X1++;
                    index = PhonemeAt(X1);

                    // next phoneme a consonant?
                    if ((Flags(index) & 64) != 0) {
                        // RULE: <VOWEL> RX | LX <CONSONANT>


if (debug) printf("RULE: <VOWEL> <RX | LX> <CONSONANT> - decrease length by 1\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", loopIndex, signInputTable1[PhonemeAt(loopIndex)], signInputTable2[PhonemeAt(loopIndex)], phonemeLength[loopIndex]);

                        // decrease length of vowel by 1 frame
                        phonemeLength[loopIndex]--;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", loopIndex, signInputTable1[PhonemeAt(loopIndex)], signInputTable2[PhonemeAt(loopIndex)], phonemeLength[loopIndex]);

                    }
                    // move ahead
                    loopIndex++;
                    continue;
                }
                // move ahead
                loopIndex++;
                continue;
            }


            // Got here if not <VOWEL>

            // not voiced
            if ((mem56 & 4) == 0)
            {

                 // Unvoiced
                 // *, .*, ?*, ,*, -*, DX, S*, SH, F*, TH, /H, /X, CH, P*, T*, K*, KX

                // not an unvoiced plosive?
                if((mem56 & 1) == 0) {
                    // move ahead
                    loopIndex++;
                    continue;
                }

                // P*, T*, K*, KX


                // RULE: <VOWEL> <UNVOICED PLOSIVE>
                // <VOWEL> <P*, T*, K*, KX>

                // move back
                X1--;

if (debug) printf("RULE: <VOWEL> <UNVOICED PLOSIVE> - decrease vowel by 1/8th\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)],  phonemeLength[X1]);

                // decrease length by 1/8th
                mem56 = phonemeLength[X1] >> 3;
                phonemeLength[X1] -= mem56;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);

                // move ahead
                loopIndex++;
                continue;
            }

            // RULE: <VOWEL> <VOICED CONSONANT>
            // <VOWEL> <WH, R*, L*, W*, Y*, M*, N*, NX, DX, Q*, Z*, ZH, V*, DH, J*, B*, D*, G*, GX>

if (debug) printf("RULE: <VOWEL> <VOICED CONSONANT> - increase vowel by 1/2 + 1\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1-1, signInputTable1[PhonemeAt(X1-1)], signInputTable2[PhonemeAt(X1-1)],  phonemeLength[X1-1]);

            // decrease length
            A1 = phonemeLength[X1-1];
            phonemeLength[X1-1] = (A1 >> 2) + A1 + 1;     // 5/4*A + 1

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1-1, signInputTable1[PhonemeAt(X1-1)], signInputTable2[PhonemeAt(X1-1)], phonemeLength[X1-1]);

            // move ahead
            loopIndex++;
            continue;

        }


        // WH, R*, L*, W*, Y*, M*, N*, NX, Q*, Z*, ZH, V*, DH, J*, B*, D*, G*, GX

//pos48821:

        // RULE: <NASAL> <STOP CONSONANT>
        //       Set punctuation length to 6
        //       Set stop consonant length to 5

        // nasal?
        if((Flags2(index) & 8) != 0)
        {

            // M*, N*, NX,

            // get the next phoneme
            X1++;
            index = PhonemeAt(X1);

            // end of buffer?
            if (index == 255)
               A1 = 65&2;  //prevent buffer overflow
            else
                A1 = Flags(index) & 2; // check for stop consonant


            // is next phoneme a stop consonant?
            if (A1 != 0)

               // B*, D*, G*, GX, P*, T*, K*, KX

            {
if (debug) printf("RULE: <NASAL> <STOP CONSONANT> - set nasal = 5, consonant = 6\n");
if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);
if (debug) printf("phoneme %td (%c%c) length %d\n", X1-1, signInputTable1[PhonemeAt(X1-1)], signInputTable2[PhonemeAt(X1-1)], phonemeLength[X1-1]);

                // set stop consonant length to 6
                phonemeLength[X1] = 6;

                // set nasal length to 5
                phonemeLength[X1-1] = 5;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);
if (debug) printf("phoneme %td (%c%c) length %d\n", X1-1, signInputTable1[PhonemeAt(X1-1)], signInputTable2[PhonemeAt(X1-1)], phonemeLength[X1-1]);

            }
            // move to next phoneme
            loopIndex++;
            continue;
        }


        // WH, R*, L*, W*, Y*, Q*, Z*, ZH, V*, DH, J*, B*, D*, G*, GX

        // RULE: <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT>
        //       Shorten both to (length/2 + 1)

        // (voiced) stop consonant?
        if((Flags(index) & 2) != 0)
        {
            // B*, D*, G*, GX

            // move past silence
            do
            {
                // move ahead
                X1++;
                index = PhonemeAt(X1);
            } while(index == 0);


            // check for end of buffer
            if (index == 255) //buffer overflow
            {
                // ignore, overflow code
                if ((65 & 2) == 0) {loopIndex++; continue;}
            } else if ((Flags(index) & 2) == 0) {
                // if another stop consonant, move ahead
                loopIndex++;
                continue;
            }

            // RULE: <UNVOICED STOP CONSONANT> {optional silence} <STOP CONSONANT>
if (debug) printf("RULE: <UNVOICED STOP CONSONANT> {optional silence} <STOP CONSONANT> - shorten both to 1/2 + 1\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);
if (debug) printf("phoneme %td (%c%c) length %d\n", X1-1, signInputTable1[PhonemeAt(X1-1)], signInputTable2[PhonemeAt(X1-1)], phonemeLength[X1-1]);
// X gets overwritten, so hold prior X value for debug statement
ptrdiff_t debugX = X1;
            // shorten the prior phoneme length to (length/2 + 1)
            phonemeLength[X1] = (phonemeLength[X1] >> 1) + 1;
            X1 = loopIndex;

            // also shorten this phoneme length to (length/2 +1)
            phonemeLength[loopIndex] = (phonemeLength[loopIndex] >> 1) + 1;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", debugX, signInputTable1[PhonemeAt(debugX)], signInputTable2[PhonemeAt(debugX)], phonemeLength[debugX]);
if (debug) printf("phoneme %td (%c%c) length %d\n", debugX-1, signInputTable1[PhonemeAt(debugX-1)], signInputTable2[PhonemeAt(debugX-1)], phonemeLength[debugX-1]);


            // move ahead
            loopIndex++;
            continue;
        }


        // WH, R*, L*, W*, Y*, Q*, Z*, ZH, V*, DH, J*, **,

        // RULE: <VOICED NON-VOWEL> <DIPHTONG>
        //       Decrease <DIPHTONG> by 2

        // liquic consonant?
        if ((Flags2(index) & 16) != 0)
        {
            // R*, L*, W*, Y*

            // get the prior phoneme
            index = PhonemeAt(X1-1);

            // prior phoneme a stop consonant>
            if((Flags(index) & 2) != 0) {
                             // Rule: <LIQUID CONSONANT> <DIPHTONG>

if (debug) printf("RULE: <LIQUID CONSONANT> <DIPHTONG> - decrease by 2\n");
if (debug) printf("PRE\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);

             // decrease the phoneme length by 2 frames (20 ms)
             phonemeLength[X1] -= 2;

if (debug) printf("POST\n");
if (debug) printf("phoneme %td (%c%c) length %d\n", X1, signInputTable1[PhonemeAt(X1)], signInputTable2[PhonemeAt(X1)], phonemeLength[X1]);
        }
         }

         // move to next phoneme
         loopIndex++;
         continue;
    }
//            goto pos48701;
}

// -------------------------------------------------------------------------
// ML : Code47503 is division with remainder, and mem50 gets the sign
void Code47503(unsigned char mem52)
{

    Y1 = 0;
    if ((mem53 & 128) != 0)
    {
        mem53 = -mem53;
        Y1 = 128;
    }
    mem50 = Y1;
    A1 = 0;
    for(X1=8; X1 > 0; X1--)
    {
        int temp = mem53;
        mem53 = mem53 << 1;
        A1 = A1 << 1;
        if (temp >= 128) A1++;
        if (A1 >= mem52)
        {
            A1 = A1 - mem52;
            mem53++;
        }
    }

    mem51 = A1;
    if ((mem50 & 128) != 0) mem53 = -mem53;

}