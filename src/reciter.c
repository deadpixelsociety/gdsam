#include <stdio.h>
#include <string.h>
#include "reciter.h"
#include "ReciterTabs.h"
#include "debug.h"
#include "sam.h"
#include <stdlib.h>
#include <stdint.h>

/* Reciter positions are independent of the renderer's byte registers. */
static ptrdiff_t A, X, Y;
static unsigned char *inputtemp;
static size_t input_length;
static char *output;
static size_t output_length, output_capacity;

static unsigned char ReadInput(ptrdiff_t position) {
    return position >= 0 && (size_t)position < input_length ? inputtemp[position] : ' ';
}

static unsigned char Classify(ptrdiff_t value) {
    return value >= 0 && (size_t)value < sizeof(tab36376) ? tab36376[value] : 0;
}

static int AppendOutput(unsigned char value) {
    char *next = SamGrow(output, &output_capacity, output_length + 2);
    if (!next) return 0;
    output = next;
    output[output_length++] = (char)value;
    output[output_length] = 0;
    return 1;
}
extern int debug;



void Code37055(ptrdiff_t mem59)
{
    X = mem59;
    X--;
    A = ReadInput(X);
    Y = A;
    A = Classify(Y);
    return;
}

void Code37066(ptrdiff_t mem58)
{
    X = mem58;
    X++;
    A = ReadInput(X);
    Y = A;
    A = Classify(Y);
}

unsigned char GetRuleByte(unsigned short mem62, unsigned char Y)
{
    unsigned int address = mem62;

    if (mem62 >= 37541)
    {
        address -= 37541;
        if (address + Y >= sizeof(rules2)) { SamSetError("Invalid reciter rule"); return 128; }
        return rules2[address+Y];
    }
    address -= 32000;
    if (address + Y >= sizeof(rules)) { SamSetError("Invalid reciter rule"); return 128; }
    return rules[address+Y];
}

static int ReciteRules(void)
{
    ptrdiff_t mem57, mem58, mem59, mem60, mem61 = -1;
    unsigned short mem62;
    unsigned char mem64, mem65, mem66;
    A = X = Y = 0;

pos36554:
    while(1)
    {
        mem61++;
        X = mem61;
        A = ReadInput(X);
        mem64 = A;
        if (A == '[')
        {
            return 1;
        }

        //pos36579:
        if (A != '.') break;
        X++;
        Y = ReadInput(X);
        A = Classify(Y) & 1;
        if(A != 0) break;
        if (!AppendOutput('.')) return 0;
    } //while


    //pos36607:
    A = mem64;
    Y = A;
    A = Classify(A);
    mem57 = A;
    if((A&2) != 0)
    {
        mem62 = 37541;
        goto pos36700;
    }

    //pos36630:
    A = mem57;
    if(A != 0) goto pos36677;
    A = 32;
    inputtemp[X] = ' ';
    if (!AppendOutput(' ')) return 0;
    goto pos36554;

pos36677:
    A = mem57 & 128;
    if(A == 0)
    {
        //36683: BRK
        return 0;
    }

    // go to the right rules for this character.
    X = mem64 - 'A';
    mem62 = tab37489[X] | (tab37515[X]<<8);

    // -------------------------------------
    // go to next rule
    // -------------------------------------

pos36700:

    // find next rule
    Y = 0;
    do
    {
        mem62 += 1;
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
    } while ((A & 128) == 0);
    Y++;

    //pos36720:
    // find '('
    while(1)
    {
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
        if (A == '(') break;
        Y++;
    }
    mem66 = Y;

    //pos36732:
    // find ')'
    do
    {
        Y++;
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
    } while(A != ')');
    mem65 = Y;

    //pos36741:
    // find '='
    do
    {
        Y++;
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
        A = A & 127;
    } while (A != '=');
    mem64 = Y;

    X = mem61;
    mem60 = X;

    // compare the string within the bracket
    Y = mem66;
    Y++;
    //pos36759:
    while(1)
    {
        mem57 = ReadInput(X);
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
        if (A != mem57) goto pos36700;
        Y++;
        if(Y == mem65) break;
        X++;
        mem60 = X;
    }

// the string in the bracket is correct

//pos36787:
    A = mem61;
    mem59 = mem61;

pos36791:
    while(1)
    {
        mem66--;
        Y = mem66;
        A = GetRuleByte(mem62, Y);
        if (SamGetError()[0]) return 0;
        mem57 = A;
        //36800: BPL 36805
        if ((A & 128) != 0) goto pos37180;
        X = A & 127;
        A = Classify(X) & 128;
        if (A == 0) break;
        X = mem59-1;
        A = ReadInput(X);
        if (A != mem57) goto pos36700;
        mem59 = X;
    }

//pos36833:
    A = mem57;
    if (A == ' ') goto pos36895;
    if (A == '#') goto pos36910;
    if (A == '.') goto pos36920;
    if (A == '&') goto pos36935;
    if (A == '@') goto pos36967;
    if (A == '^') goto pos37004;
    if (A == '+') goto pos37019;
    if (A == ':') goto pos37040;
    //  Code42041();    //Error
    //36894: BRK
    return 0;

    // --------------

pos36895:
    Code37055(mem59);
    A = A & 128;
    if(A != 0) goto pos36700;
pos36905:
    mem59 = X;
    goto pos36791;

    // --------------

pos36910:
    Code37055(mem59);
    A = A & 64;
    if(A != 0) goto pos36905;
    goto pos36700;

    // --------------


pos36920:
    Code37055(mem59);
    A = A & 8;
    if(A == 0) goto pos36700;
pos36930:
    mem59 = X;
    goto pos36791;

    // --------------

pos36935:
    Code37055(mem59);
    A = A & 16;
    if(A != 0) goto pos36930;
    A = ReadInput(X);
    if (A != 72) goto pos36700;
    X--;
    A = ReadInput(X);
    if ((A == 67) || (A == 83)) goto pos36930;
    goto pos36700;

    // --------------

pos36967:
    Code37055(mem59);
    A = A & 4;
    if(A != 0) goto pos36930;
    A = ReadInput(X);
    if (A != 72) goto pos36700;
    if ((A != 84) && (A != 67) && (A != 83)) goto pos36700;
    mem59 = X;
    goto pos36791;

    // --------------


pos37004:
    Code37055(mem59);
    A = A & 32;
    if(A == 0) goto pos36700;

pos37014:
    mem59 = X;
    goto pos36791;

    // --------------

pos37019:
    X = mem59;
    X--;
    A = ReadInput(X);
    if ((A == 'E') || (A == 'I') || (A == 'Y')) goto pos37014;
    goto pos36700;
    // --------------

pos37040:
    Code37055(mem59);
    A = A & 32;
    if(A == 0) goto pos36791;
    mem59 = X;
    goto pos37040;

//---------------------------------------


pos37077:
    X = mem58+1;
    A = ReadInput(X);
    if (A != 'E') goto pos37157;
    X++;
    Y = ReadInput(X);
    X--;
    A = Classify(Y) & 128;
    if(A == 0) goto pos37108;
    X++;
    A = ReadInput(X);
    if (A != 'R') goto pos37113;
pos37108:
    mem58 = X;
    goto pos37184;
pos37113:
    if ((A == 83) || (A == 68)) goto pos37108;  // 'S' 'D'
    if (A != 76) goto pos37135; // 'L'
    X++;
    A = ReadInput(X);
    if (A != 89) goto pos36700;
    goto pos37108;

pos37135:
    if (A != 70) goto pos36700;
    X++;
    A = ReadInput(X);
    if (A != 85) goto pos36700;
    X++;
    A = ReadInput(X);
    if (A == 76) goto pos37108;
    goto pos36700;

pos37157:
    if (A != 73) goto pos36700;
    X++;
    A = ReadInput(X);
    if (A != 78) goto pos36700;
    X++;
    A = ReadInput(X);
    if (A == 71) goto pos37108;
    //pos37177:
    goto pos36700;

    // -----------------------------------------

pos37180:

    A = mem60;
    mem58 = A;

pos37184:
    Y = mem65 + 1;

    //37187: CPY 64
    //  if(? != 0) goto pos37194;
    if(Y == mem64) goto pos37455;
    mem65 = Y;
    //37196: LDA (62),y
    A = GetRuleByte(mem62, Y);
    if (SamGetError()[0]) return 0;
    mem57 = A;
    X = A;
    A = Classify(X) & 128;
    if(A == 0) goto pos37226;
    X = mem58+1;
    A = ReadInput(X);
    if (A != mem57) goto pos36700;
    mem58 = X;
    goto pos37184;
pos37226:
    A = mem57;
    if (A == 32) goto pos37295;   // ' '
    if (A == 35) goto pos37310;   // '#'
    if (A == 46) goto pos37320;   // '.'
    if (A == 38) goto pos37335;   // '&'
    if (A == 64) goto pos37367;   // ''
    if (A == 94) goto pos37404;   // ''
    if (A == 43) goto pos37419;   // '+'
    if (A == 58) goto pos37440;   // ':'
    if (A == 37) goto pos37077;   // '%'
    //pos37291:
    //  Code42041(); //Error
    //37294: BRK
    return 0;

    // --------------
pos37295:
    Code37066(mem58);
    A = A & 128;
    if(A != 0) goto pos36700;
pos37305:
    mem58 = X;
    goto pos37184;

    // --------------

pos37310:
    Code37066(mem58);
    A = A & 64;
    if(A != 0) goto pos37305;
    goto pos36700;

    // --------------


pos37320:
    Code37066(mem58);
    A = A & 8;
    if(A == 0) goto pos36700;

pos37330:
    mem58 = X;
    goto pos37184;

    // --------------

pos37335:
    Code37066(mem58);
    A = A & 16;
    if(A != 0) goto pos37330;
    A = ReadInput(X);
    if (A != 72) goto pos36700;
    X++;
    A = ReadInput(X);
    if ((A == 67) || (A == 83)) goto pos37330;
    goto pos36700;

    // --------------


pos37367:
    Code37066(mem58);
    A = A & 4;
    if(A != 0) goto pos37330;
    A = ReadInput(X);
    if (A != 72) goto pos36700;
    if ((A != 84) && (A != 67) && (A != 83)) goto pos36700;
    mem58 = X;
    goto pos37184;

    // --------------

pos37404:
    Code37066(mem58);
    A = A & 32;
    if(A == 0) goto pos36700;
pos37414:
    mem58 = X;
    goto pos37184;

    // --------------

pos37419:
    X = mem58;
    X++;
    A = ReadInput(X);
    if ((A == 69) || (A == 73) || (A == 89)) goto pos37414;
    goto pos36700;

// ----------------------

pos37440:

    Code37066(mem58);
    A = A & 32;
    if(A == 0) goto pos37184;
    mem58 = X;
    goto pos37440;
pos37455:
    Y = mem64;
    mem61 = mem60;

    if (debug)
        PrintRule(mem62);

pos37461:
    //37461: LDA (62),y
    A = GetRuleByte(mem62, Y);
    if (SamGetError()[0]) return 0;
    mem57 = A;
    A = A & 127;
    if (A != '=')
    {
        if (!AppendOutput((unsigned char)A)) return 0;
    }

    //37478: BIT 57
    //37480: BPL 37485  //not negative flag
    if ((mem57 & 128) == 0) goto pos37485; //???
    goto pos36554;
pos37485:
    Y++;
    goto pos37461;
}

int ReciteText(const char *text, size_t length, char **phonemes, size_t *phoneme_length) {
    int ok;
    *phonemes = NULL;
    *phoneme_length = 0;
    output = NULL;
    output_length = output_capacity = 0;
    if (length > PTRDIFF_MAX - 2) { SamSetError("Text is too large"); return 0; }
    input_length = length + 2;
    inputtemp = SamAlloc(input_length);
    if (!inputtemp) return 0;
    inputtemp[0] = ' ';
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126 || c == '[' || c == ']') {
            SamSetError("Unsupported text byte at position %zu", i);
            free(inputtemp); inputtemp = NULL; return 0;
        }
        inputtemp[i + 1] = c >= 'a' && c <= 'z' ? c - 32 : c;
    }
    inputtemp[length + 1] = '[';
    ok = ReciteRules();
    free(inputtemp); inputtemp = NULL;
    if (!ok) {
        SamSetError("Unable to recite text near position %td", X > 0 ? X - 1 : 0);
        free(output); output = NULL; return 0;
    }
    *phonemes = output;
    *phoneme_length = output_length;
    output = NULL;
    return 1;
}

/* Compatibility entry point for bounded, sentinel-terminated C callers. */
int TextToPhonemes(unsigned char *text) {
    size_t length = 0, result_length;
    char *result;
    while (length < 256 && text[length] && text[length] != '[') ++length;
    if (length == 256) { SamSetError("Unterminated reciter input"); return 0; }
    if (!ReciteText((const char *)text, length, &result, &result_length)) return 0;
    if (result_length > 253) {
        free(result); SamSetError("Use ReciteText for expanded phoneme output"); return 0;
    }
    memcpy(text, result, result_length);
    text[result_length] = 155;
    text[result_length + 1] = 0;
    free(result);
    return 1;
}
