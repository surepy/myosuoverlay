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

    //  offset 7
    static constexpr unsigned char TIME[] = { 0xDB, 0x5D, 0xE8, 0x8B, 0x45, 0xE8, 0xA3 };;
    static constexpr char *TIME_MASK = PCHAR("xxxxxxx");
    static constexpr int TIME_OFFSET = 6 + 1; // + 1 goes to address that points to the currentAudioTime value
};