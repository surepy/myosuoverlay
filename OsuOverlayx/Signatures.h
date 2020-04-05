#pragma once
#include <Windows.h>
#include "OsuGame.h"

class osuGame;

//  note: https://github.com/Piotrekol/StreamCompanion/blob/171fab829d921aa9bae355904d8bb90c0eba0e47/plugins/OsuMemoryEventSource/MemoryListener.cs
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

        return NULL; // no sig found
    }

    // got from external sources :)

    /*  frame delay (fps)
    #=zq_nW7Nx1FUE8jqy1BA==::#=zDtFubb0=+F7 - D9E8                  - fld1
    #=zq_nW7Nx1FUE8jqy1BA==::#=zDtFubb0=+F9 - DEE1                  - fsubrp st(1),st(0)
    #=zq_nW7Nx1FUE8jqy1BA==::#=zDtFubb0=+FB - DC 0D 08605303        - fmul qword ptr [03536008] { [33.01] } <<
    #=zq_nW7Nx1FUE8jqy1BA==::#=zDtFubb0=+101- DEC1                  - faddp
    #=zq_nW7Nx1FUE8jqy1BA==::#=zDtFubb0=+103- DD 5E 0C              - fstp qword ptr [esi+0C]

    \xdd\x45\x00\xd9\xe8\xde\xe1\xdc\x0d xx?xxxxxx
    dd 45 ?
    d9 e8
    de e1
    dc 0d

    */
    static constexpr unsigned char FRAME_DELAY[] = { 0xdd, 0x45, 0x00, 0xd9, 0xe8, 0xde, 0xe1, 0xdc, 0x0d };
    static constexpr char *FRAME_DELAY_MASK = PCHAR("xx?xxxxxx");
    static constexpr int FRAME_DELAY_OFFSET = 8 + 1;

    /*
        These sigs are from:
        https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/3a48045f9686075d67c9533a06f34d98a285afaf/OsuMemoryDataProvider/OsuMemoryReader.cs#L34
    */
    static constexpr unsigned char OSU_BASE[] = { 0xF8, 0x01, 0x74, 0x04, 0x83 };
    static constexpr char *OSU_BASE_MASK = PCHAR("xxxxx");
    static constexpr int OSU_BASE_OFFSET = 0;

    static constexpr int GAMEMODE_OFFSET = -51;
    static constexpr int RETRYS_PTR_OFFSET[] = { 8 };

    static constexpr unsigned char STATUS[] = { 0x48, 0x83, 0xF8, 0x04, 0x73, 0x1E };
    static constexpr char *STATUS_MASK = PCHAR("xxxxxx");
    static constexpr int STATUS_OFFSET = 0;

    static constexpr unsigned char TIME[] = { 0x5E, 0x5F, 0x5D, 0xC3, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x89, 0x00, 0x04 };
    static constexpr char *TIME_MASK = PCHAR("xxxxx????x?x");
    static constexpr int TIME_OFFSET = 5;

    static constexpr int BEATMAP_DATA_OFFSET = -12; // NOTE: double ponter

    // NOTE: double pointer
    static constexpr unsigned char PLAYDATA[] = { 0x85, 0xC9, 0x74, 0x1F, 0x8D, 0x55, 0xF0, 0x8B, 0x01 };
    static constexpr char *PLAYDATA_MASK = PCHAR("xxxxxxxxx");
    static constexpr int PLAYDATA_OFFSET = -4;
    //85 C9 74 1F 8D 55 F0 8B 01

    /*
    \x80\x3d\x00\x00\x00\x00\x00\x75\x00\x8b\x7e xx?????x?xx

    80 3d ? ? ? ? ? 75 ? 8b 7e
    */

    static constexpr unsigned char INPUT_COUNTER[] = { 0xFF, 0x46, 0x14, 0x80, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x00, 0x8b, 0x7e };
    static constexpr char *INPUT_COUNTER_MASK = PCHAR("xxxxx?????x?xx");
    static constexpr int INPUT_COUNTER_OFFSET = 4 + 1;

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

    new sig:

    1B384D00 - DB 5D EC  - fistp dword ptr [ebp-14]
    1B384D03 - 8B 45 EC  - mov eax,[ebp-14]
    1B384D06 - A3 CC67A601 - mov [01A667CC],eax <<
    1B384D0B - 83 3D 4C407804 00 - cmp dword ptr [0478404C],00
    1B384D12 - 74 1E - je #=zsb859T37qVe3NOvX1Q==::#=z$TuaImGpnw3D+472

    EAX=000092B2
    EBX=00000000
    ECX=273F7E8C
    EDX=00000000
    ESI=00000000
    EDI=188A6BF8
    ESP=016FEBD8
    EBP=016FEC04
    EIP=1B384D0B

    \xdb\x5d\x00\x8b\x45\x00\xa3\x00\x00\x00\x00\x83\x3d xx?xx?x????xx

    */

    //static constexpr unsigned char TIME[] = { 0xDB, 0x5D, 0xE8, 0x8B, 0x45, 0xE8, 0xA3 };
    static constexpr unsigned char TIME_BACKUP[] = { 0xDB, 0x5D, 0xEC, 0x8B, 0x45, 0xEC, 0xA3 };
    static constexpr char *TIME_BACKUP_MASK = PCHAR("xxxxxxx");
    static constexpr int TIME_BACKUP_OFFSET = 6 + 1; // + 1 goes to address that points to the currentAudioTime value

    /*[Backup] Mods

    see Mods struct

    0C61DD9B - 8B 45 B8  - mov eax,[ebp-48]
    0C61DD9E - 8B 48 14  - mov ecx,[eax+14]
    0C61DDA1 - 09 0D 68629301  - or [01936268],ecx <<
    0C61DDA7 - E8 9494FDFF - call #=z7S$_NECUeKvDcG$Jt6s7VjIpwiJVqlw20n0y1h8=::#=z9NWjQ9pbrv8Pn7$A$A==
    0C61DDAC - 8B 45 BC  - mov eax,[ebp-44]

    EAX=1B2432F8
    EBX=00000000
    ECX=00000001
    EDX=00000000
    ESI=0153EDAC
    EDI=0153EDBC
    ESP=0153ED94
    EBP=0153EDDC
    EIP=0C61DDA7

    \xff\xe0\x8b\x45\x00\x8b\x48\x00\x09\x0d xxxx?xx?xx

    \xff\xe0\x8b\x45\x00\x8b\x48\x00\x09\x0d xxxx?xx?xx
    \xff\xe0\x8b\x45\x00\x8b\x48\x00\x09\x0d xxxx?xx?xx
    */

    static constexpr unsigned char MODS[] = { 0xFF, 0xE0, 0x8B, 0x45, 0x00, 0x8B, 0x48, 0x00, 0x09, 0x0D };
    static constexpr char *MODS_MASK = PCHAR("xxxx?xx?xx");
    static constexpr int MODS_OFFSET = 9 + 1;

    /* [BACKUP]
    Game mode global (0 = osu!, 1 = osu!taiko, 2 = osu!catch, 3 = osu!mania)

    0A2DFEB8 - 18 81 450E0000        - sbb [ecx+00000E45],al
    0A2DFEBE - 00 00                 - add [eax],al
    0A2DFEC0 - 0C 81                 - or al,-7F { 129 }
    0A2DFEC2 - 45                    - inc ebp
    0A2DFEC3 - 0E                    - push cs
    0A2DFEC4 - EC                    - in al,dx
    0A2DFEC5 - AB                    - stosd
    0A2DFEC6 - 7C 06                 - jl #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+6

    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$ - 55                    - push ebp
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+1- 8B EC                 - mov ebp,esp
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+3- 57                    - push edi
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+4- 56                    - push esi
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+5- 53                    - push ebx
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+6- 3B 0D F45CB902        - cmp ecx,[02B95CF4] { (0) }
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+C- 74 60                 - je #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+6E
    #=z9Hok5U34HsdnWn9JHJXpJ60N8uhI::#=zbrxSYSObQkm$+E- 89 0D F45CB902        - mov [02B95CF4],ecx { (0) }

    sig:
    \x55\x8b\xec\x57\x56\x53\x3b\x0d xxxxxxxx

    \xec\x57\x56\x53\x3b\x0d xxxxxx

    55 8B EC 57 56 53 3B 0D F4 5C B9 02
    */

    static constexpr unsigned char GAMEMODE_GLOBAL[] = { 0xEC, 0x57, 0x56, 0x53, 0x3B, 0x0D };
    static constexpr char *GAMEMODE_GLOBAL_MASK = PCHAR("xxxxxx");
    static constexpr int GAMEMODE_GLOBAL_OFFSET = 5 + 1;
};