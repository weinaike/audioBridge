#ifndef AUDIOBRIDGE_AUDIOLEVELS_H
#define AUDIOBRIDGE_AUDIOLEVELS_H

namespace audiobridge {

// Audio level information for monitoring
struct AudioLevels {
    float inputPeakL = 0.0f;   // Left channel input peak [0.0, 1.0]
    float inputPeakR = 0.0f;   // Right channel input peak
    float outputPeakL = 0.0f;  // Left channel output peak
    float outputPeakR = 0.0f;  // Right channel output peak

    /// Reset all levels to zero
    void Reset() {
        inputPeakL = 0.0f;
        inputPeakR = 0.0f;
        outputPeakL = 0.0f;
        outputPeakR = 0.0f;
    }

    /// Get maximum input level
    float GetMaxInputLevel() const {
        return (inputPeakL > inputPeakR) ? inputPeakL : inputPeakR;
    }

    /// Get maximum output level
    float GetMaxOutputLevel() const {
        return (outputPeakL > outputPeakR) ? outputPeakL : outputPeakR;
    }
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_AUDIOLEVELS_H
