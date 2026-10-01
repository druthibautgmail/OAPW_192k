#pragma once

#include <cmath>
#include <algorithm>

class Biquad {
public:
    enum Type {
        LOWPASS,
        HIGHPASS,
        HIGHSHELF // NEU: Für das Head-Shadowing Modell
    };

    Biquad() {
        reset();
        setCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    }

    void setCoefficients(double b0, double b1, double b2, double a0, double a1, double a2) {
        double invA0 = 1.0 / a0;
        b0_ = b0 * invA0;
        b1_ = b1 * invA0;
        b2_ = b2 * invA0;
        a1_ = a1 * invA0;
        a2_ = a2 * invA0;
    }

    void setButterworth(Type type, double cutoffFreq, double sampleRate, double Q = 0.70710678) {
        if (cutoffFreq <= 0.0 || cutoffFreq >= sampleRate * 0.5) {
            setCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
            return;
        }

        double w0 = 2.0 * M_PI * cutoffFreq / sampleRate;
        double cosW0 = std::cos(w0);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 / (2.0 * Q);

        double b0 = 0.0, b1 = 0.0, b2 = 0.0;
        double a0 = 1.0, a1 = 0.0, a2 = 0.0;

        if (type == LOWPASS) {
            b0 = (1.0 - cosW0) / 2.0;
            b1 = 1.0 - cosW0;
            b2 = (1.0 - cosW0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
        } else if (type == HIGHPASS) {
            b0 = (1.0 + cosW0) / 2.0;
            b1 = -(1.0 + cosW0);
            b2 = (1.0 + cosW0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
        }

        setCoefficients(b0, b1, b2, a0, a1, a2);
    }

    // NEU: Berechnung der Koeffizienten für einen High-Shelf Filter
    void setHighShelf(double cutoffFreq, double sampleRate, double gainDB, double Q = 0.70710678) {
        if (cutoffFreq <= 0.0 || cutoffFreq >= sampleRate * 0.5) {
            setCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
            return;
        }
        
        double A = std::pow(10.0, gainDB / 40.0);
        double w0 = 2.0 * M_PI * cutoffFreq / sampleRate;
        double cosW0 = std::cos(w0);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 / (2.0 * Q);

        double b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha);
        double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
        double b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha);
        double a0 = (A + 1.0) - (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha;
        double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
        double a2 = (A + 1.0) - (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha;

        setCoefficients(b0, b1, b2, a0, a1, a2);
    }

    inline double process(double in) {
        // Direct Form II Transposed (Höchste numerische Stabilität)
        double out = b0_ * in + z1_;
        z1_ = b1_ * in - a1_ * out + z2_;
        z2_ = b2_ * in - a2_ * out;
        return out;
    }

    void reset() {
        z1_ = 0.0;
        z2_ = 0.0;
    }

private:
    double b0_, b1_, b2_;
    double a1_, a2_;
    double z1_, z2_;
};
