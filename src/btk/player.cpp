#include <atomic>
#include <stdint.h>
#include <iostream>
#include "RtAudio.h"
#include "Blip_Buffer.h"
#include "ofl_gbapu.h"

typedef Blip_Synth<blip_good_quality, -450> SynthSqr;
typedef Blip_Synth<blip_med_quality,  -450> SynthWav;
typedef Blip_Synth<blip_med_quality,  -450> SynthNoi;

static struct Player
{
    ofl_gbapu apu;
    SynthSqr sqr1, sqr2;
    SynthWav wav;
    SynthNoi noi;
    RtAudio dac;
    RtAudio::StreamParameters stream_params;
    RtAudio::StreamOptions stream_opt;
    uint32_t sample_rate;
    uint32_t buffer_frames;
} p;

void ErrorCallback(RtAudioError::Type type, const std::string &errorText)
{
    //std::cerr << "RtAudio Error: " << errorText << "\n";
}

int PlaybackCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
    double streamTime, RtAudioStreamStatus status, void *userData)
{
    unsigned int i;
    float *buffer = (float*)outputBuffer;
    static float lastValues[2] = { 0 };
    if (status)
        std::cout << "Stream underflow detected!" << std::endl;
    for (i = 0; i<nBufferFrames; i++) {
        *buffer++ = lastValues[0];
        lastValues[0] += 0.005 * (0 + 1 + (0*0.1));
        if (lastValues[0] >= 1.0) lastValues[0] -= 2.0;
    }
    for (i = 0; i<nBufferFrames; i++) {
        *buffer++ = lastValues[1];
        lastValues[1] += 0.005 * (1 + 1 + (1*0.1));
        if (lastValues[1] >= 1.0) lastValues[1] -= 2.0;
    }
    return 0;
}

void Player_Init(void)
{
    ofl_gbapu_init(&p.apu);

    p.dac.showWarnings(true);
    p.sample_rate = 44100;
    p.buffer_frames = 512;
    p.stream_params.nChannels = 2;
    p.stream_opt.flags = RTAUDIO_NONINTERLEAVED;
    if (p.dac.getDeviceCount() > 0)
    {
        p.stream_params.deviceId = p.dac.getDefaultOutputDevice();
        p.dac.openStream(&p.stream_params, 0, RTAUDIO_FLOAT32, p.sample_rate, &p.buffer_frames, PlaybackCallback, 0, &p.stream_opt, ErrorCallback);
        p.dac.startStream();
    }
}
