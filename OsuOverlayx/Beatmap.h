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
    int32_t get_y(bool hardrock) {
        return hardrock ? 384 - this->y : this->y;
    }


    slidercurve(int32_t x, int32_t y) : x{ x }, y { y }
    {
    
    }

    slidercurve(DirectX::SimpleMath::Vector2 vec) : x{ static_cast<int32_t>(vec.x) }, y{ static_cast<int32_t>(vec.y) }
    {

    }

    // don't use this tho
    slidercurve() : x{ 0 }, y{ 0 }
    {
    
    }

    bool operator==(slidercurve *curve)
    {
        return this->x == curve->x && this->y == curve->y;
    }

    bool operator==(slidercurve &curve)
    {
        return this->x == curve.x && this->y == curve.y;
    }

    operator DirectX::SimpleMath::Vector2*() const {
        DirectX::SimpleMath::Vector2 a = DirectX::SimpleMath::Vector2(x, y);
        return &a;
    }

    operator DirectX::SimpleMath::Vector2() const {
        return DirectX::SimpleMath::Vector2(x, y);
    }

    DirectX::SimpleMath::Vector2 vec2() {
        return static_cast<DirectX::SimpleMath::Vector2>(*this);
    }

    operator slidercurve() const {
        return slidercurve(x, y);
    }

    slidercurve slider_curve() {
        return static_cast<slidercurve>(*this);
    }
};

struct hitobject {
    uint8_t type;
    int32_t x;
    int32_t y;
    int32_t get_y(bool hardrock) {
        return hardrock ? 384 - this->y : this->y;
    }

    // 
    std::wstring definition;

    // do slidercurves.back()!
    slidercurve sliderend;

    std::vector<slidercurve> slidercurves;
    std::vector<slidercurve> slidercurves_calculated;
    std::wstring slidertype;

    // streaming until x notes passed.
    int32_t streaming_until = 0;

    int32_t start_time;
    int32_t end_time;
    // more like slides - repeat + 1 but ok
    uint32_t repeat;
    uint32_t pixel_length;

    bool operator==(hitobject *obj)
    {
        return this->x == obj->x && this->y == obj->y && this->start_time == obj->start_time
            && this->end_time == obj->end_time && this->type == obj->type;
    }

    int32_t subtract_start_time(hitobject sub) {
        return this->start_time - sub.start_time;
    }

    int32_t subtract_start_time(int32_t sub) {
        return this->start_time - sub;
    }

    // these functions are dumb, like really dumb.
    // why no operator-()? because we also need end_time funcs.
    int32_t subtract_start_time_inv(int32_t sub) {
        return sub - this->start_time;
    }

    int32_t subtract_end_time(hitobject sub) {
        return this->end_time - sub.end_time;
    }

    int32_t subtract_end_time(int32_t sub) {
        return this->end_time - sub;
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

    /// <summary>
    /// if slider: gets the real "end curve" of the slider, meaning it respects the repeat counts.
    /// if not, just return self.
    /// </summary>
    /// <returns></returns>
    slidercurve calculated_back_real() {
        if (this->repeat % 2 == 0 || !this->IsSlider()) {
            return slidercurve(this->x, this->y);
        }

        return this->slidercurves_calculated.back();
    }

    operator DirectX::SimpleMath::Vector2() const {
        return DirectX::SimpleMath::Vector2(x, y);
    }

    DirectX::SimpleMath::Vector2 vec2() {
        return static_cast<DirectX::SimpleMath::Vector2>(*this);
    }

    operator slidercurve() const {
        return slidercurve(x, y);
    }
};

struct timingpoint {
    int32_t offset;
    float velocity;
    float ms_per_beat;
    bool kiai;
    bool inherited;

    int32_t subtract_offset(timingpoint sub) {
        return this->offset - sub.offset;
    }

    int32_t subtract_offset(int32_t sub) {
        return this->offset - sub;
    }

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
    bool ParseHitObject(std::vector<std::wstring>& values, const std::wstring & definition);
    bool ParseHitObjectSlider(hitobject &object);
    bool ParseDifficultySettings(std::wstring difficulty_line);
    bool ParseGeneral(std::wstring difficulty_line);
    bool ParseMetaData(std::wstring difficulty_line);

    std::size_t index_temp;
public:
    bool Parse(const std::wstring filename, const std::wstring md5hash);

    void Unload();

    // the map itself
public:
    int BeatMapID = 0;
    int BeatmapSetID = 0;
    std::wstring MD5_Hash;
    std::wstring Title;
    std::wstring Artist;
    std::wstring Creator;
    std::wstring Version;
    int Mode;

    bool loaded;
    
    //  note: this is amount of rows in mania.
    float CircleSize; 
    float ApproachRate;
    float SliderMultiplier;

    std::vector<hitobject> hitobjects;
    std::vector<timingpoint> timingpoints;

    // move to overlay.h
    std::vector<hitobject> aspire_dumb_slider_hitobjects;

    std::vector<hitobject> hitobjects_sorted[10];   // mostly for mania

    // for use in actually drawing
public:
    /*
        standard macros
    */
    hitobject* getHitObject(size_t index) {
        if (index >= this->hitobjects.size())
            return nullptr;
        return &this->hitobjects.at(index);
    }

    hitobject* getCurrentHitObject() {
        return this->getHitObject(this->currentObjectIndex);
    };

    hitobject* getNextHitObject() {
        return this->getHitObject(this->currentObjectIndex + 1);
    };

    hitobject* getUpcomingHitObject() {
        return this->getHitObject(this->currentObjectIndex + 2);
    };

    /*
        mania macros
    */
    hitobject* getHitObject(size_t row, size_t index) {
        if (index >= this->hitobjects_sorted[row].size())
            return nullptr;
        return &this->hitobjects_sorted[row][index];
    }

    hitobject* getCurrentHitObject(size_t row)
    {
        return this->getHitObject(row, this->currentObjectIndex_sorted[row]);
    };

    hitobject* getNextHitObject(int row)
    {
        return this->getHitObject(row, this->currentObjectIndex_sorted[row] + 1);
    };

    hitobject* getUpcomingHitObject(int row)
    {
        return this->getHitObject(row, this->currentObjectIndex_sorted[row] + 2);
    };

    timingpoint* getTimingPoint(size_t index) {
        if (index >= this->timingpoints.size())
            return nullptr;
        return &this->timingpoints.at(index);
    }

    timingpoint* getCurrentUninheritedTimingPoint(int offset = 0)
    {
        return this->getTimingPoint(this->currentUninheritTimeIndex + offset);
    };

    timingpoint* getCurrentTimingPoint(int offset = 0)
    {
        return this->getTimingPoint(this->currentTimeIndex + offset);
    };

    // consider refactoring, but you're probably too lazy to do so.
    // indexes for objects
    void resetIndexes() {
        for (size_t& i : this->currentObjectIndex_sorted)
        {
            i = 0;
        }
        this->currentUninheritTimeIndex = 0;
        this->currentTimeIndex = 0;
        this->currentObjectIndex = 0;
        this->newComboIndex = 0;
        this->currentSpeed = 0;
        this->kiai = false;
    }

    std::size_t currentObjectIndex = 0;
    std::size_t currentObjectIndex_sorted[10]; // mania only
    std::size_t currentUninheritTimeIndex = 0;
    std::size_t currentTimeIndex = 0;
    std::size_t newComboIndex = 0;
    std::uint32_t currentBpm;
    float currentSpeed;
    bool kiai;
};