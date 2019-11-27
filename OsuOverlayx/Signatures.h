#pragma once
#include <Windows.h>
#include "OsuGame.h"

struct osuGame;

struct Signatures
{
public:
    //  https://aixxe.net/2016/10/osu-game-hacking
    //  https://github.com/aixxe/osu-relax/blob/master/relax/src/findpattern.h
    //  https://github.com/CookieHoodie/OsuBot/blob/e7564502cce16c8dab3d6b67e478d78d0fce35df/OsuBots/SigScanner.cpp
    //  blindly copypasted for now, learn later.
    //  function needs changes later for sigs that look like "93 ?? 9D AC ??"
    static DWORD FindPattern(HANDLE processHandle, const unsigned char pattern[], const char* mask, const int offset, size_t begin = 0) {
        // pattern in format: unsigned char pattern[] = { 0x90, 0xFF, 0xEE };
        // mask in format: char* mask = "xxx?xxx";
        // begin default is 0
        // this function searches the signature from begin or 0x00000000(default) to 0x7FFFFFFF
        const size_t signature_size = strlen(mask);
        const size_t read_size = 4096;
        bool hit = false;

        unsigned char chunk[read_size];

        for (size_t i = begin; i < INT_MAX; i += read_size - signature_size) {
            ReadProcessMemory(processHandle, LPCVOID(i), &chunk, read_size, NULL);

            for (size_t a = 0; a < read_size; a++) {
                hit = true;

                for (size_t j = 0; j < signature_size && hit; j++) {
                    if (mask[j] != '?' && chunk[a + j] != pattern[j]) {
                        hit = false;
                    }
                }

                if (hit) {
                    return i + a + offset;
                }
            }
        }

        return NULL;
    }

    // got from external sources :)
    /*  time
    08E3DF26 - DB 5D E8  - fistp dword ptr [ebp-18]
    08E3DF29 - 8B 45 E8  - mov eax,[ebp-18]
    08E3DF2C - A3 345D2C01 - mov [012C5D34],eax <<
    08E3DF31 - 8B 35 F843CB03  - mov esi,[03CB43F8]
    08E3DF37 - 85 F6  - test esi,esi

    EAX=0002202C
    EBX=00000000
    ECX=37A56CAC
    EDX=00000000
    ESI=00000000
    EDI=1CC0A110
    ESP=009CEA04
    EBP=009CEA34
    EIP=08E3DF31

    \xdb\x5d\x00\x8b\x45\x00\xa3\x00\x00\x00\x00\x8b\x35 xx?xx?x????xx
    */

    static constexpr unsigned char TIME[] = { 0xDB, 0x5D, 0xE8, 0x8B, 0x45, 0xE8, 0xA3 };
    static constexpr char *TIME_MASK = PCHAR("xxxxxxx");
    static constexpr int TIME_OFFSET = 6 + 1; // + 1 goes to address that points to the currentAudioTime value

    /*  frame delay (fps)
    08E35B0B - DC 0D 48632C01  - fmul qword ptr [012C6348]
    08E35B11 - DEC1 - faddp
    08E35B13 - DD 5E 0C  - fstp qword ptr [esi+0C] <<
    08E35B16 - 8B 4E 34  - mov ecx,[esi+34]
    08E35B19 - 39 09  - cmp [ecx],ecx

    EAX=00000001
    EBX=02CF51A0
    ECX=02D9D9E4
    EDX=00003F80
    ESI=02D9D9E4
    EDI=02CBD14C
    ESP=009CEAE0
    EBP=009CEAFC
    EIP=08E35B16

    \xdd\x5e\x00\x8b\x4e xx?xx

    */
    static constexpr unsigned char FRAME_DELAY[] = { 0xdd, 0x5e, 0x0c, 0x8b, 0x4e };
    static constexpr char *FRAME_DELAY_MASK = PCHAR("xx?xx");
    static constexpr int FRAME_DELAY_OFFSET = 4 + 0x0C;

    /*  score
    09DB8FBA - 5B - pop ebx
    09DB8FBB - 05 60F2D000 - add eax,00D0F260
    09DB8FC0 - 89 51 28  - mov [ecx+28],edx <<
    09DB8FC3 - C3 - ret
    09DB8FC4 - 00 00  - add [eax],al

    EAX=00D0F2B8
    EBX=00000000
    ECX=2B80C368
    EDX=00048B74
    ESI=21674438
    EDI=2B80C368
    ESP=009CE82C
    EBP=009CE844
    EIP=09DB8FC3

    \x89\x51\x00\xc3 xx?x

    89 51 ? c3
    */

    /* map name (i think)
    779C7F8B - 83 E2 03 - and edx,03
    779C7F8E - 83 F9 08 - cmp ecx,08
    779C7F91 - 72 29 - jb ntdll.memmove+5C <<
    779C7F93 - F3 A5 - repe movsd
    779C7F95 - FF 24 95 AC809C77  - jmp dword ptr [edx*4+ntdll.memmove+14C]

    EAX=064CEA06
    EBX=020800BE
    ECX=0000001A
    EDX=00000002
    ESI=064CE99C
    EDI=0C18E824
    ESP=064CE8E4
    EBP=064CE8EC
    EIP=779C7F93

    */
};