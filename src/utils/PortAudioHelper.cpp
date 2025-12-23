#include "utils/PortAudioHelper.h"
#include "utils/Logger.h"
#include <stdexcept>

namespace audiobridge {

PortAudioHelper& PortAudioHelper::GetInstance() {
    static PortAudioHelper instance;
    return instance;
}

bool PortAudioHelper::Initialize() {
    if (initialized_) {
        refCount_++;
        AB_LOG_DEBUG("PortAudioHelper: Ref count incremented to " + std::to_string(refCount_));
        return true;
    }

    AB_LOG_INFO("PortAudioHelper: Initializing PortAudio");
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        AB_LOG_ERROR(std::string("PortAudioHelper: Failed to initialize PortAudio: ") +
                     Pa_GetErrorText(err));
        return false;
    }

    initialized_ = true;
    refCount_ = 1;
    AB_LOG_INFO("PortAudioHelper: PortAudio initialized successfully");
    AB_LOG_INFO(std::string("PortAudioHelper: Version: ") + Pa_GetVersionText());
    return true;
}

void PortAudioHelper::Terminate() {
    if (refCount_ > 0) {
        refCount_--;
        AB_LOG_DEBUG("PortAudioHelper: Ref count decremented to " + std::to_string(refCount_));
    }

    if (refCount_ == 0 && initialized_) {
        AB_LOG_INFO("PortAudioHelper: Terminating PortAudio");
        PaError err = Pa_Terminate();
        if (err != paNoError) {
            AB_LOG_ERROR(std::string("PortAudioHelper: Failed to terminate PortAudio: ") +
                         Pa_GetErrorText(err));
        } else {
            AB_LOG_INFO("PortAudioHelper: PortAudio terminated");
        }
        initialized_ = false;
    }
}

bool PortAudioHelper::IsInitialized() const {
    return initialized_;
}

PortAudioHelper::~PortAudioHelper() {
    // Ensure PortAudio is terminated when the helper is destroyed
    if (initialized_) {
        Pa_Terminate();
    }
}

}  // namespace audiobridge
