#ifndef AUDIOBRIDGE_DUMMYENGINE_H
#define AUDIOBRIDGE_DUMMYENGINE_H

#include "core/Types.h"

namespace audiobridge {

/// Placeholder processing engine for pass-through mode
class DummyEngine {
public:
    DummyEngine() = default;
    ~DummyEngine() = default;

    /// Process audio (pass-through implementation)
    /// @param input Input buffer
    /// @param output Output buffer
    void Process(const AudioBuffer& input, AudioBuffer& output);
};

}  // namespace audiobridge

#endif  // AUDIOBRIDGE_DUMMYENGINE_H
