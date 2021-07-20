/**
 *  @file   Audio.h
 *  @brief  MacOS Audio Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-19
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef AUDIO_H_
#define AUDIO_H_

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

inline OSStatus OutsideRenderer(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber, UInt32 inNumberFrames,
                                AudioBufferList *ioData);

class Audio {

public:
  Audio();
  ~Audio();

  void audioStart();
  void audioStop();
  void audioClear();

  void loadSoundFromFile(const char *filename);

  void playSound(char sample, char cont = 0);

  int getVolume();
  void setVolume(int volume);

  OSStatus audioRenderer(void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                         UInt32 inNumberFrames, AudioBufferList *ioData);

  bool OK();

private:
  AudioComponent output_comp;

  AudioComponentInstance output_instance;

  bool audioOn;

  unsigned int samplerate;
  unsigned int channels;
  unsigned int stereo;

  typedef struct {
    int size;
    char *data;
  } Sample;

  Sample *samples;
  int n_samples;

  typedef struct {
    int pos;
    char sample;
    char loop;
  } AudioChannel;

  AudioChannel *mixer;
};

inline bool Audio::OK() { return Audio::audioOn; }

#define VOLUME_RANGE (40) /* decibels */

inline int Audio::getVolume() {
  if (!audioOn)
    return 0;
  float factor;
  AudioUnitGetParameter(output_instance, kHALOutputParam_Volume,
                        kAudioUnitScope_Output, 0, &factor);
  if (factor == 0.0)
    return 0;
  int volume = 100 + log10f(factor) * 20 * 100 / (float)VOLUME_RANGE;
  return volume;
}

inline void Audio::setVolume(int volume) {
  if (!audioOn)
    return;
  float factor =
      (volume == 0) ? 0.0
                    : powf(10, (float)VOLUME_RANGE * (volume - 100) / 100 / 20);
  AudioUnitSetParameter(output_instance, kHALOutputParam_Volume,
                        kAudioUnitScope_Output, 0, factor, 0);
}

#endif // End of AUDIO_H_
