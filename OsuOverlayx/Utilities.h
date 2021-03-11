#pragma once

class Utilities {
public:
    static std::wstring s2ws(const std::string& str);

    static const DWORD getProcessIDbyName(const wchar_t* name);

    static std::wstring ReadWStringFromMemory(HANDLE hnd, DWORD ptr);

    struct MappingFile;

    static uint64_t factorial(int n)
    {
        if (n < 0)
            throw new std::range_error("no");
        if (n == 0)
            return 1;
        else
            return n * factorial(n - 1);
    }

    static uint32_t binomialCoeff(int n, int k)
    {
        int res = 1;

        // Since C(n, k) = C(n, n-k)
        if (k > n - k)
            k = n - k;

        // Calculate value of
        // [n * (n-1) *---* (n-k+1)] / [k * (k-1) *----* 1]
        for (int i = 0; i < k; ++i)
        {
            res *= (n - i);
            res /= (i + 1);
        }

        return res;
    }

    static DirectX::SimpleMath::Vector2 object_completion(const DirectX::SimpleMath::Vector2 &p1, const DirectX::SimpleMath::Vector2 &p2, double completion);

    static double time_completion_percent(double osu_time, double start_time, double end_time);

    static double getBezierPoint(std::vector<int> *points, double t) {
        double answer = 0;
        int n = points->size() - 1;

        for (int i = 0; i < points->size(); i++)
        {
            answer += (binomialCoeff(n, i) *  std::pow((1 - t), (n - i))  * std::pow(t, i) * (double)points->at(i));
        }

        return answer;
    }

    static DirectX::SimpleMath::Vector2 ClampVector2Magnitude(DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &vector, double length)
    {
        double vec_length = DirectX::SimpleMath::Vector2::Distance(origin, vector);

        if (vec_length > length) {
            return origin + ((vector - origin) * (length / vec_length));
        }
        return vector;
    }

    // pasted
    // https://github.com/ppy/osu/blob/3391e21fc48a9f3a4e2505649ce7a235c250a909/osu.Game/Rulesets/Objects/SliderPath.cs#L231
    static double osu_Game_Rulesets_Objects_SliderPath_calculateLength(std::vector<DirectX::SimpleMath::Vector2> calculatedPath, double pixel_length);

    static std::wstring to_wstring_with_precision(const double a_value, const int n = 6);

    // not pasted from 
    // https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    template<typename ... Args>
    static std::wstring swprintf_s(const std::wstring& format, Args ... args) {
        int size = swprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size <= 0) { throw std::runtime_error("Error during formatting."); }
        std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
        swprintf(buf.get(), size, format.c_str(), args ...);
        return std::wstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    static std::chrono::milliseconds time_since_epoch() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        );
    }
};