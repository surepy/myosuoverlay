#include "pch.h"
#include "Beatmap.h"
#include "OsuOverlay.h"
#include "OsuGame.h"
#include <algorithm>

inline bool split_line(std::wstring line, wchar_t delimiter, std::vector<std::wstring>& result) {
    std::wstringstream input_line(line);
    std::wstring current_line;

    while (std::getline(input_line, current_line, delimiter)) {
        if (!current_line.empty()) {
            result.push_back(current_line);
        }
    }

    return (result.size() >= 2);
};

inline bool beatmap::GetTimingPointFromOffset(uint32_t offset, timingpoint& target_point) {
    if (this->timingpoints.size() == 0) {
        return false;
    }

    for (size_t i = this->timingpoints.size(); i-- > 0;) {
        if (this->timingpoints.at(i).offset <= offset) {
            target_point = this->timingpoints.at(i);
            return true;
        }
    }

    target_point = this->timingpoints.front();
    return true;
}

inline bool beatmap::ParseTimingPoint(std::vector<std::wstring>& values) {
    static float last_ms_per_beat;
    
    // osu file format v6 fix...
    try {
        bool kiai = std::stoi(values.at(7)) == 1;
    }
    catch (std::out_of_range& e) {
        bool kiai = false;
    }

    int32_t offset;
    try {
        offset = std::stoi(values.at(0));
    }
    catch (const std::out_of_range& e)
    {
        UNREFERENCED_PARAMETER(e);
        offset = -1;
    }

    float ms_per_beat;
    bool inherited = false;

    try {
        ms_per_beat = std::stof(values.at(1));
    }
    catch (const std::out_of_range& e)
    {
        UNREFERENCED_PARAMETER(e);
        ms_per_beat = 1;
    }

    float velocity = 1.f;

    if (ms_per_beat < 0) {
        inherited = true;
        velocity = abs(100 / ms_per_beat);

        if (last_ms_per_beat != 0) {
            ms_per_beat = last_ms_per_beat;
        }
    }
    else {
        last_ms_per_beat = ms_per_beat;
    }

    timingpoint new_timingpoint;

    new_timingpoint.offset = offset;
    new_timingpoint.velocity = velocity;
    new_timingpoint.ms_per_beat = ms_per_beat;
    new_timingpoint.kiai = kiai;
    new_timingpoint.inherited = inherited;

    this->timingpoints.push_back(new_timingpoint);

    return true;
}

inline void CalculateBezierPts(hitobject& object, std::vector<int> &x_points, std::vector<int> &y_points, double &dist_left)
{
    int x, y;

    DirectX::SimpleMath::Vector2 prev_point = DirectX::SimpleMath::Vector2(x_points.front(), y_points.front());

    //  https://gamedev.stackexchange.com/questions/5373/moving-ships-between-two-planets-along-a-bezier-missing-some-equations-for-acce/5427#5427
    for (double t = 0; t <= 1; t += 0.01)
    {
        x = Utilities::getBezierPoint(&x_points, t);
        y = Utilities::getBezierPoint(&y_points, t);

        if (DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y)) < 10 && t != 0)
        {
            continue;
        }

        dist_left -= DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y));

        if (dist_left < 0)
            break;

        prev_point = DirectX::SimpleMath::Vector2(x, y);

        object.slidercurves_calculated.push_back(slidercurve(x, y));
    }

    x_points.clear();
    y_points.clear();
}

bool beatmap::ParseHitObjectSlider(hitobject& object)
{
    if (object.slidercurves.size() == 0)
    {
        OutputDebugString(std::wstring(L"WARNING | beatmap::ParseHitObjectSlider(): object @ " + std::to_wstring(object.start_time) + L" has no slider curves! wtf??\n").c_str());
        OutputDebugString(std::wstring(L"adding self.point so program doesn't crash...\n").c_str());
        object.slidercurves_calculated.push_back(slidercurve(object.x, object.y));
        return false;
    }
    if (object.slidertype == L"B")
    {
        double length_left = object.pixel_length;
        int size = object.slidercurves.size();

        //  segment size (pixels) btw
        double segment_length = 20;

        std::vector<int> x_points, y_points;
        x_points.push_back(object.x);
        y_points.push_back(object.y);

        DirectX::SimpleMath::Vector2 prev_point;

        for (int i = 0; i < size; i++)
        {
            x_points.push_back(object.slidercurves.at(i).x);
            y_points.push_back(object.slidercurves.at(i).y);

            // new bezier line.
            if (i + 1 < object.slidercurves.size() && object.slidercurves.at(i) == object.slidercurves.at(i + 1))
            {
                CalculateBezierPts(object, x_points, y_points, length_left);
            }
        }

        CalculateBezierPts(object, x_points, y_points, length_left);

        // line connection temp
        //if (object.slidercurves_calculated.back().x != Utilities::getBezierPoint(&x_points, 1.0) && object.slidercurves_calculated.back().y != Utilities::getBezierPoint(&y_points, 1.0))
        //{
        //    object.slidercurves_calculated.push_back(slidercurve(Utilities::getBezierPoint(&x_points, 1.0), Utilities::getBezierPoint(&y_points, 1.0)));
        //}
    }
    else if (object.slidertype == L"L")
    {
        DirectX::SimpleMath::Vector2 first_curve = DirectX::SimpleMath::Vector2(object.x, object.y);
        double dist_left = object.pixel_length;

        for (int i = 0; i < object.slidercurves.size(); i++)
        {
            // THIS CRASHES ON ASPIRE DUMB MAPS IF LENGTH IS 0 ON A SLIDER
            // WHY
            // so we append at minimum 1 slidercurve.
            if (dist_left < 0 && i != 0)
                break;

            DirectX::SimpleMath::Vector2 curve_point = object.slidercurves.at(i);

            DirectX::SimpleMath::Vector2 vec_final = Utilities::ClampVector2Magnitude(first_curve, curve_point, dist_left);

            object.slidercurves_calculated.push_back(vec_final);

            dist_left -= DirectX::SimpleMath::Vector2::Distance(first_curve, vec_final);

            first_curve = vec_final;
        }
    }
    else if (object.slidertype == L"P")
    {
        // https://github.com/ppy/osu/blob/a8579c49f9327b0a4a8278a76167e081d14ea516/osu.Game/Rulesets/Objects/CircularArcApproximator.cs
        // copy-paste modification
        if (object.slidercurves.size() != 2)
        {
            throw new std::invalid_argument("More or Less than 3 Points in DrawSliderPerfectCircle! what!?");
        }

        // fix of weird-ass behaivor of calculated point going -(int32 max).
        // check if slope is same or no slope & same x axis and use linear calculation if so.
        // holy shit this code is bad.
        bool treat_as_linear = false;

        // no slope, and same x axis.
        if (object.slidercurves.front().x == object.x && object.slidercurves.back().x == object.slidercurves.front().x) {
            treat_as_linear = true;
        }
        // same slope.
        else {
            double slope_one = static_cast<double>(object.slidercurves.front().y - object.y) / static_cast<double>(object.slidercurves.front().x - object.x);
            double slope_two = static_cast<double>(object.slidercurves.back().y - object.slidercurves.front().y) / static_cast<double>(object.slidercurves.back().x - object.slidercurves.front().x);

            treat_as_linear = slope_one == slope_two;
        }


        if (treat_as_linear) {
            OutputDebugString(std::wstring(L"WARNING | beatmap::ParseHitObjectSlider(): object @ " + std::to_wstring(object.start_time) + L" treating as linear!\n").c_str());
            object.slidertype = L"L";
            object.slidercurves.erase(object.slidercurves.begin());
            return ParseHitObjectSlider(object);
        }

        const long double pi = (atan(1) * 4);

        DirectX::SimpleMath::Vector2 first = DirectX::SimpleMath::Vector2(object.x, object.y),  // first
            second = static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.at(0)), // passthrough
            third = static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.at(1));  // end

        double aSq = DirectX::SimpleMath::Vector2::DistanceSquared(second, third);
        double bSq = DirectX::SimpleMath::Vector2::DistanceSquared(first, third);
        double cSq = DirectX::SimpleMath::Vector2::DistanceSquared(first, second);

        // degenerate triangle i'mma ignore until things break

        double s = aSq * (bSq + cSq - aSq);
        double t = bSq * (aSq + cSq - bSq);
        double u = cSq * (aSq + bSq - cSq);

        float sum = s + t + u;

        // again, see above.

        DirectX::SimpleMath::Vector2 centre = (s * first + t * second + u * third) / sum;
        DirectX::SimpleMath::Vector2 dA = first - centre;
        DirectX::SimpleMath::Vector2 dC = third - centre;

        float r = dA.Length();  // is this the correct func?

        double thetaStart = std::atan2(dA.y, dA.x);
        double thetaEnd = std::atan2(dC.y, dC.x);

        while (thetaEnd < thetaStart)
            thetaEnd += 2 * pi;

        double dir = 1;
        double thetaRange = thetaEnd - thetaStart;

        // Decide in which direction to draw the circle, depending on which side of
        // AC B lies.
        DirectX::SimpleMath::Vector2 orthoAtoC = third - first;
        orthoAtoC = DirectX::SimpleMath::Vector2(orthoAtoC.y, -orthoAtoC.x);
        if (orthoAtoC.Dot(second - first) < 0)
        {
            dir = -dir;
            thetaRange = 2 * pi - thetaRange;
        }

        const float tolerance = 0.1f;

        // We select the amount of points for the approximation by requiring the discrete curvature
        // to be smaller than the provided tolerance. The exact angle required to meet the tolerance
        // is: 2 * Math.Acos(1 - TOLERANCE / r)
        // The special case is required for extremely short sliders where the radius is smaller than
        // the tolerance. This is a pathological rather than a realistic case.
        int amountPoints = 2 * r <= tolerance ? 2 : std::max(2, (int)std::ceil(thetaRange / (2 * std::acos(1 - tolerance / r))));

        DirectX::SimpleMath::Vector2 last_point = object.slidercurves.back();

        for (int i = 0; i < amountPoints; ++i)
        {
            double fract = (double)i / (amountPoints - 1);
            double theta = thetaStart + dir * fract * thetaRange;
            DirectX::SimpleMath::Vector2 o = DirectX::SimpleMath::Vector2((float)std::cos(theta), (float)std::sin(theta)) * r;

            // calculate total arch length, if overshoot. stop.
            if (std::abs((dir * fract * thetaRange) * r) >= object.pixel_length)
            {
                break;
            }

            DirectX::SimpleMath::Vector2 point = centre + o;

            slidercurve curve(point.x, point.y);

            object.sliderend = curve;
            object.slidercurves_calculated.push_back(curve);
        }
    }
    else if (object.slidertype == L"C")
    {
        // TODO
    }
    else
    {
        // wtf
    }
    return true;
}

inline bool beatmap::ParseHitObject(std::vector<std::wstring>& values, const std::wstring & definition) {
    hitobject new_hitobject;

    new_hitobject.definition = definition;
    new_hitobject.type = std::stoi(values.at(3));
    new_hitobject.x = std::stoi(values.at(0));
    new_hitobject.y = std::stoi(values.at(1));
    new_hitobject.start_time = std::stoi(values.at(2));

    if (new_hitobject.IsCircle()) {
        new_hitobject.end_time = new_hitobject.start_time;

        if (Mode == (int)PlayMode::MANIA)
        {
            int row = static_cast<int>(std::clamp(std::floor(new_hitobject.x * this->CircleSize / 512), 0.f, CircleSize - 1));
            this->hitobjects_sorted[row].push_back(new_hitobject);
        }
    }
    else if (new_hitobject.IsSpinner()) {
        new_hitobject.end_time = std::stoi(values.at(5));
    }
    else if (new_hitobject.IsSlider()) {
        std::vector<std::wstring> objectparams;

        split_line(values.at(5), L'|', objectparams);

        new_hitobject.slidertype = objectparams.at(0);

        for (int i = 1; i < objectparams.size(); i++)
        {
            std::vector<std::wstring> point;
            slidercurve new_slidercurve;

            split_line(objectparams.at(i), L':', point);

            new_slidercurve.x = std::stoi(point.at(0));
            new_slidercurve.y = std::stoi(point.at(1));

            new_hitobject.slidercurves.push_back(new_slidercurve);
        }

        new_hitobject.repeat = std::stoi(values.at(6));
        new_hitobject.pixel_length = std::stoi(values.at(7));

        ParseHitObjectSlider(new_hitobject);

        timingpoint object_timingpoint;

        if (this->GetTimingPointFromOffset(new_hitobject.start_time, object_timingpoint)) {
            float px_per_beat = this->SliderMultiplier * 100 * object_timingpoint.velocity;
            float beats_num = (new_hitobject.pixel_length * new_hitobject.repeat) / px_per_beat;
            new_hitobject.end_time = static_cast<int32_t>(new_hitobject.start_time + ceil(beats_num * object_timingpoint.ms_per_beat));
        }
    }
    else if (new_hitobject.IsHold())
    {
        // mania only
        std::vector<std::wstring> objectparams;

        split_line(values.at(5), L':', objectparams);

        new_hitobject.end_time = std::stoi(objectparams[0]);

        int row = static_cast<int>(std::clamp(std::floor(new_hitobject.x * this->CircleSize / 512), 0.f, CircleSize - 1));
        this->hitobjects_sorted[row].push_back(new_hitobject);
    }

    // thanks to https://osu.ppy.sh/beatmapsets/594828#osu/1258033
    // the hitobjects are all over the fucking place motherfuck

    if (this->hitobjects.size() == 0)
    {
        this->hitobjects.push_back(new_hitobject);
        return true;
    }

    for (int i = 0; i < this->hitobjects.size(); i++)
    {
        if (this->hitobjects.at(i).start_time > new_hitobject.start_time)
        {
            this->hitobjects.insert(this->hitobjects.begin() + i, new_hitobject);
            return true;
        }
    }

    this->hitobjects.push_back(new_hitobject);
    return true;
}

inline bool beatmap::ParseGeneral(std::wstring difficulty_line) {
    std::vector<std::wstring> values;

    if (split_line(difficulty_line, ':', values)) {
        if (!_wcsicmp(values[0].c_str(), L"Mode")) {
            this->Mode = std::stoi(values[1]);
        }
    }

    return true;
};

inline bool beatmap::ParseDifficultySettings(std::wstring difficulty_line) {
    std::vector<std::wstring> values;

    if (split_line(difficulty_line, ':', values)) {
        if (!_wcsicmp(values[0].c_str(), L"slidermultiplier")) {
            this->SliderMultiplier = std::stof(values[1]);
        }
        else if (!_wcsicmp(values[0].c_str(), L"CircleSize")) {
            this->CircleSize = std::stof(values[1]);
        }
        else if (!_wcsicmp(values[0].c_str(), L"ApproachRate")) {
            this->ApproachRate = std::stof(values[1]);
        }
    }

    return true;
}

inline bool beatmap::ParseMetaData(std::wstring difficulty_line) {
    std::vector<std::wstring> values;

    if (split_line(difficulty_line, ':', values)) {
        if (!_wcsicmp(values[0].c_str(), L"BeatmapID")) {
            this->BeatMapID = std::stoi(values[1]);
        }
        else if (!_wcsicmp(values[0].c_str(), L"BeatmapSetID")) {
            this->BeatmapSetID = std::stoi(values[1]);
        }
        /*else if (!_wcsicmp(values[0].c_str(), L"Artist")) {
            this->Artist = values[1];
        }
        else if (!_wcsicmp(values[0].c_str(), L"Title")) {
            //this->Title = values[1];
        }
        else if (!_wcsicmp(values[0].c_str(), L"Creator")) {
            this->Creator = values[1];
        }
        else if (!_wcsicmp(values[0].c_str(), L"Version")) {
            this->Version = values[1];
        }*/
    }
    return true;
}

bool beatmap::Parse(const std::wstring filename, const std::wstring md5hash) {
    std::wifstream beatmap_file(filename, std::ifstream::in);

    if (!beatmap_file.good()) {
        return false;
    }

    std::wstring current_line;
    MD5_Hash = md5hash;

    while (std::getline(beatmap_file, current_line)) {
        static std::wstring current_section;

        if (!current_line.empty() && current_line.front() == '[' && current_line.back() == ']') {
            current_section = current_line.substr(1, current_line.length() - 2);
            continue;
        }

        if (current_section.empty()) {
            continue;
        }

        if (!current_section.compare(L"General"))
        {
            this->ParseGeneral(current_line);
        }
        else if (!current_section.compare(L"Difficulty")) {
            this->ParseDifficultySettings(current_line);
        }
        else if (!current_section.compare(L"Metadata")) {
            this->ParseMetaData(current_line);
        }
        else {
            std::vector<std::wstring> values;

            if (!split_line(current_line, L',', values)) {
                continue;
            }

            if (!current_section.compare(L"Events"))
            {
            }
            else if (!current_section.compare(L"TimingPoints")) {
                this->ParseTimingPoint(values);
            }
            else if (!current_section.compare(L"HitObjects")) {
                this->ParseHitObject(values, current_line);
            }
        }
    }

    return true;
}

void beatmap::Unload()
{
    if (!this->loaded)
    {
        return;
    }

    this->BeatMapID = 0;
    this->BeatmapSetID = 0;
    this->Title = L"";
    this->Artist = L"";
    this->Creator = L"";
    this->Version = L"";
    this->Mode = 0;

    this->loaded = false;

    this->CircleSize = 0;
    this->SliderMultiplier = 0;

    this->hitobjects.clear();
    this->timingpoints.clear();

    for (std::vector<hitobject> &row : this->hitobjects_sorted)
    {
        row.clear();
    }

    this->currentObjectIndex = 0;
    for (size_t &i : currentObjectIndex_sorted)
    {
        i = 0;
    }
    this->currentUninheritTimeIndex = 0;
    this->currentTimeIndex = 0;
    this->newComboIndex = 0;
    this->currentBpm = 0;
    this->currentSpeed = 0;
    this->kiai = false;
}