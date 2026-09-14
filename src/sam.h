#ifndef SAM_H
#define SAM_H
#include <stddef.h>
#include <stdint.h>

void *SamAlloc(size_t size);
void *SamGrow(void *data, size_t *capacity, size_t needed);
void SamSetError(const char *format, ...);
const char *SamGetError(void);
/* The engine owns input, phonemes, and PCM. Reset starts a call; FreeSam ends it. */
void FreeSam(void);
int SetInputText(const char *text, size_t length);
int SamWriteAudio(int advance, const unsigned char *samples, size_t count);
#ifdef SAM_TESTING
void SamFailAllocationAfter(ptrdiff_t count);
#endif

void ResetSam();
void SetInput(char *_input);
void SetSpeed(unsigned char _speed);
void SetPitch(unsigned char _pitch);
void SetMouth(unsigned char _mouth);
void SetThroat(unsigned char _throat);
void EnableSingmode();
void EnableDebug();

int SAMMain();

/* Borrowed PCM; never free this pointer directly. Length is in SAM timing ticks. */
char* GetBuffer();
uint64_t GetBufferLength();


//char input[]={"/HAALAOAO MAYN NAAMAEAE IHSTT SAEBAASTTIHAAN \x9b\x9b\0"};
//unsigned char input[]={"/HAALAOAO \x9b\0"};
//unsigned char input[]={"AA \x9b\0"};
//unsigned char input[] = {"GUH5DEHN TAEG\x9b\0"};

//unsigned char input[]={"AY5 AEM EY TAO4LXKIHNX KAX4MPYUX4TAH. GOW4 AH/HEH3D PAHNK.MEYK MAY8 DEY.\x9b\0"};
//unsigned char input[]={"/HEH3LOW2, /HAW AH YUX2 TUXDEY. AY /HOH3P YUX AH FIYLIHNX OW4 KEY.\x9b\0"};
//unsigned char input[]={"/HEY2, DHIHS IH3Z GREY2T. /HAH /HAH /HAH.AYL BIY5 BAEK.\x9b\0"};
//unsigned char input[]={"/HAH /HAH /HAH \x9b\0"};
//unsigned char input[]={"/HAH /HAH /HAH.\x9b\0"};
//unsigned char input[]={".TUW BIY5Y3,, OHR NAA3T - TUW BIY5IYIY., DHAE4T IHZ DHAH KWEH4SCHAHN.\x9b\0"};
//unsigned char input[]={"/HEY2, DHIHS \x9b\0"};

//unsigned char input[]={" IYIHEHAEAAAHAOOHUHUXERAXIX  \x9b\0"};
//unsigned char input[]={" RLWWYMNNXBDGJZZHVDH \x9b\0"};
//unsigned char input[]={" SSHFTHPTKCH/H \x9b\0"};

//unsigned char input[]={" EYAYOYAWOWUW ULUMUNQ YXWXRXLX/XDX\x9b\0"};


#endif
