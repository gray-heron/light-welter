
#include "spectrum.h"

float VisibleSpectrum::AverageSpectrumSamples(float lambda_low, float lambda_high,
                                              const std::vector<LightSample> &samples)
{
    float ret = 0;
    const float span = (lambda_high - lambda_low);

    if (lambda_high <= samples[0].lambda_)
        return samples[0].value_;
    if (lambda_low >= (samples.end() - 1)->lambda_)
        return (samples.end() - 1)->value_;
    if (samples.size() == 1)
        return samples[0].value_;

    if (lambda_low < samples[0].lambda_)
        ret += samples[0].value_ * span;
    if (lambda_high > (samples.end() - 1)->lambda_)
        ret += (samples.end() - 1)->value_ * span;

    unsigned int i = 0;
    for (; lambda_low > samples[i + 1].lambda_; ++i)
        ;

    auto interpolate = [samples](float lambda, int sample_id) {
        return lwmath::lerp(
            (lambda - samples[sample_id].lambda_) /
                (samples[sample_id + 1].lambda_ - samples[sample_id].lambda_),
            samples[sample_id].value_, samples[sample_id + 1].value_);
    };  

    for (; i + 1 < samples.size() && lambda_high >= samples[i].lambda_; ++i)
    {
        float segment_low = std::max(lambda_low, samples[i].lambda_);
        float segment_high = std::min(lambda_high, samples[i + 1].lambda_);

        ret += 0.5f * (interpolate(segment_low, i) + interpolate(segment_high, i)) *
               (segment_high - segment_low);
    }

    return ret / span;
}