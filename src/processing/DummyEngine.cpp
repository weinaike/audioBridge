#include "processing/DummyEngine.h"
#include <cstring>

namespace audiobridge {

void DummyEngine::Process(const AudioBuffer& input, AudioBuffer& output) {
    // Pass-through: copy input directly to output
    if (input.data && output.data && input.frameCount == output.frameCount &&
        input.channelCount == output.channelCount) {
        std::memcpy(output.data, input.data,
                    input.frameCount * input.channelCount * sizeof(float));
    }
}

}  // namespace audiobridge
