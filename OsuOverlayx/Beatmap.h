#pragma once

#include <vector>
#include <fstream>
#include <sstream>

enum ObjectType : uint8_t {
    HITOBJECT_CIRCLE = 1,
    HITOBJECT_SLIDER = 2,
    HITOBJECT_NEW_COMBO = 4,
    HITOBJECT_SPINNER = 8,
    HITOBJECT_MANIA_HOLD = 128
};

struct slidercurve {
    int32_t x;
    int32_t y;

    bool operator==(slidercurve *curve)
    {
        return this->x == curve->x && this->y == curve->y;
    }

    bool operator==(slidercurve &curve)
    {
        return this->x == curve.x && this->y == curve.y;
    }

    operator DirectX::SimpleMath::Vector2*() const {
        return &DirectX::SimpleMath::Vector2(x, y);
    }

    operator DirectX::SimpleMath::Vector2() const {
        return DirectX::SimpleMath::Vector2(x, y);
    }
};

struct hitobject {
    uint8_t type;
    int32_t x;
    int32_t y;
    int32_t start_time;
    int32_t end_time;
    // more like slides - repeat  + 1 but ok
    uint32_t repeat;
    uint32_t pixel_length;

    bool operator==(hitobject *obj)
    {
        return this->x == obj->x && this->y == obj->y && this->start_time == obj->start_time
            && this->end_time == obj->end_time && this->type == obj->type;
    }

    bool IsCircle() const {
        return (type & HITOBJECT_CIRCLE) == HITOBJECT_CIRCLE;
    }

    bool IsSlider() const {
        return (type & HITOBJECT_SLIDER) == HITOBJECT_SLIDER;
    }

    bool IsHold() const {
        return (type & HITOBJECT_MANIA_HOLD) == HITOBJECT_MANIA_HOLD;
    }

    bool IsSpinner() const {
        return (type & HITOBJECT_SPINNER) == HITOBJECT_SPINNER;
    }

    bool IsNewCombo() const {
        return (type & HITOBJECT_NEW_COMBO) == HITOBJECT_NEW_COMBO;
    }

    operator DirectX::SimpleMath::Vector2() const {
        return DirectX::SimpleMath::Vector2(x, y);
    }

    std::vector<slidercurve> slidercurves;
    std::wstring slidertype;

    int32_t x_sliderend_real;
    int32_t y_sliderend_real;
};

struct timingpoint {
    int32_t offset;
    float velocity;
    float ms_per_beat;
    bool kiai;
    bool inherited;

    int getBPM() {
        return static_cast<int>(60000 / ms_per_beat);
    }

    double getBPM_Double()
    {
        return 60000 / ms_per_beat;
    }

    bool operator==(timingpoint const &point)
    {
        return (this->velocity == point.velocity && this->kiai == point.kiai && this->ms_per_beat == point.ms_per_beat && this->inherited);
        //
    }

    bool operator!=(timingpoint const &point)
    {
        return !(*this == point);
    }
};

class beatmap {
private:
    bool GetTimingPointFromOffset(uint32_t offset, timingpoint& target_point);

    bool ParseTimingPoint(std::vector<std::wstring>& values);
    bool ParseHitObject(std::vector<std::wstring>& values);
    bool ParseDifficultySettings(std::wstring difficulty_line);
    bool ParseGeneral(std::wstring difficulty_line);
    bool ParseMetaData(std::wstring difficulty_line);
public:
    bool Parse(const std::wstring filename, const std::wstring md5hash);

    void Unload();

    // the map itself
public:
    int BeatMapID = 0;
    int BeatmapSetID = 0;
    std::wstring MD5_Hash;
    /*    std::wstring Title;
        std::wstring Artist;
        std::wstring Creator;
        std::wstring Version;*/
    int Mode;

    bool loaded;

    float CircleSize; //  note: this is amount of rows in mania.
    float ApproachRate;
    float SliderMultiplier;

    std::vector<hitobject> hitobjects;
    std::vector<timingpoint> timingpoints;

    std::vector<hitobject> aspire_dumb_slider_hitobjects;

    std::vector<hitobject> hitobjects_sorted[10];   // mostly for mania

    // for use in actually drawing
public:
    /*
        standard macros
    */
    hitobject* getCurrentHitObject() {
        try {
            return &this->hitobjects[(size_t)this->currentObjectIndex];
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    hitobject* getNextHitObject() {
        try {
            return &this->hitobjects.at((size_t)this->currentObjectIndex + 1);
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    hitobject* getUpcomingHitObject() {
        try {
            return &this->hitobjects.at((size_t)this->currentObjectIndex + 2);
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    /*
        mania macros
    */
    hitobject* getCurrentHitObject(int row)
    {
        try {
            return &this->hitobjects_sorted[row][(size_t)this->currentObjectIndex_sorted[row]];
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    hitobject* getNextHitObject(int row)
    {
        try {
            return &this->hitobjects_sorted[row].at((size_t)this->currentObjectIndex_sorted[row] + 1);
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    hitobject* getUpcomingHitObject(int row)
    {
        try {
            return &this->hitobjects_sorted[row].at((size_t)this->currentObjectIndex_sorted[row] + 2);
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    timingpoint* getCurrentTimingPoint()
    {
        try {
            return &this->timingpoints.at((size_t)this->currentTimeIndex);
        }
        catch (const std::out_of_range& e)
        {
            (e);
            return nullptr;
        }
    };

    std::uint32_t currentObjectIndex = 0;
    std::uint32_t currentObjectIndex_sorted[10]; // mania only
    std::uint32_t currentUninheritTimeIndex = 0;
    std::uint32_t currentTimeIndex = 0;
    std::uint32_t newComboIndex = 0;
    std::uint32_t currentBpm;
    float currentSpeed;
    bool kiai;
};