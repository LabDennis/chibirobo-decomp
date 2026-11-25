#ifndef DOLPHIN_SDK_MATH_H
#define DOLPHIN_SDK_MATH_H

static inline float SDKFastSqrt(float x)
{
    static const double half = 0.5;
    static const double three = 3.0;

    if (x > 0.0f) {
        double estimate = __frsqrte((double)x);
        // Refine the reciprocal square root before converting the result to float.
        estimate = half * estimate * (three - estimate * estimate * x);
        estimate = half * estimate * (three - estimate * estimate * x);
        estimate = half * estimate * (three - estimate * estimate * x);
        return (float)(x * estimate);
    }
    return x;
}

#endif
