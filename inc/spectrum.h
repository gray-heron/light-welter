
#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "build_config.h"
#include "lwmath.h"

template <int N_SAMPLES> class CoefficientSpectrum
{
    std::array<float, N_SAMPLES> samples_;

  public:
    CoefficientSpectrum(float value = 0.0f)
    {
        for (int i = 0; i < samples_; ++i)
            samples_[i] = value;
    }

    CoefficientSpectrum &operator+=(const CoefficientSpectrum &rhs)
    {
        for (int i = 0; i < N_SAMPLES; ++i)
            samples_[i] += rhs.samples_[i];
        return *this;
    }

    CoefficientSpectrum operator+(const CoefficientSpectrum &rhs) const
    {
        CoefficientSpectrum ret;
        for (int i = 0; i < N_SAMPLES; ++i)
            ret.c[i] = samples_[i] + rhs.samples_[i];
        return ret;
    }

    float &operator[](int i) { return samples_[i]; }
};

struct LightSample
{
    float lambda_;
    float value_;
};

class VisibleSpectrum : CoefficientSpectrum<SPECTRUM_N_SAMPLES>
{

    float AverageSpectrumSamples(float lambda_low, float lambda_high,
                                 const std::vector<LightSample> &samples);

  public:
    VisibleSpectrum(float initial);
};