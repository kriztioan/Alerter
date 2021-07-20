/**
 *  @file   Audio.cpp
 *  @brief  Linux Audio Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-19
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Audio.h"

Audio::Audio() {
  Audio::mixer = NULL;
  Audio::samples = NULL;
  Audio::n_samples = 0;
  Audio::audioOn = false;
  Audio::channels = 16;
  Audio::stereo = 0;
  Audio::samplerate = 11025;
  Audio::blocksize = 512;
  Audio::audioStart();
}

Audio::~Audio() {

  if (Audio::audioOn)
    Audio::audioStop();
}

void Audio::audioClear() {

  if (Audio::writefd > -1) {
    AudioMessage msg = {.type = SOUND_CLEAR, .size = 0};
    write(Audio::writefd, &msg, sizeof(AudioMessage));
  }

  if (Audio::samples) {

    for (int i = 0; i < Audio::n_samples; i++) {

      if (samples[i].data) {
        delete[] samples[i].data;
        samples[i].data = NULL;
      }
    }

    free(Audio::samples);
    Audio::n_samples = 0;
    Audio::samples = NULL;
  }
}

void Audio::audioStop() {

  if (Audio::writefd > -1) {
    AudioMessage msg = {.type = SOUND_EXIT, .size = 0};
    write(Audio::writefd, &msg, sizeof(AudioMessage));
    wait(NULL);
    close(Audio::writefd);
    Audio::audioOn = false;
    Audio::writefd = -1;
  }

  delete Audio::device;
  Audio::device = NULL;

  Audio::audioClear();
}

void Audio::audioStart() {

  Audio::device = new char[9];
  strcpy(Audio::device, "/dev/dsp");

  int fd[2];
  if (pipe(fd) != 0) {
    Audio::audioOn = false;
    return;
  }

  Audio::readfd = fd[0];
  Audio::writefd = fd[1];
  if (fork() != 0) {
    if (access(Audio::device, R_OK | W_OK) == 0)
      Audio::audioOn = true;
    close(Audio::readfd);
    return;
  }

  close(Audio::writefd);
  Audio::dsp = open(Audio::device, O_WRONLY);

  if (Audio::dsp < 0) {
    Audio::audioServer();
  }

  int format = AFMT_S16_LE;
  ioctl(Audio::dsp, SNDCTL_DSP_SETFMT, &format);
  ioctl(Audio::dsp, SNDCTL_DSP_STEREO, &(Audio::stereo));
  ioctl(Audio::dsp, SNDCTL_DSP_SPEED, &(Audio::samplerate));

  fcntl(Audio::readfd, F_SETFL, O_NONBLOCK);

  if (access(Audio::device, W_OK) == 0)
    Audio::audioOn = true;
  else
    Audio::audioOn = false;

  Audio::audioServer();
}

void Audio::loadSoundFromFile(const char *filename) {

  int fd = open(filename, O_RDONLY), size;

  if (fd == -1)
    return;

  Audio::samples = (Sample *)realloc(Audio::samples,
                                     (Audio::n_samples + 1) * sizeof(Sample));

  Audio::samples[Audio::n_samples].size = size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  samples[Audio::n_samples].data = new char[size];
  read(fd, samples[Audio::n_samples].data,
       Audio::samples[Audio::n_samples].size);

  char *tmp = samples[Audio::n_samples++].data;
  while (size--)
    *tmp++ -= 127;

  close(fd);
}

void Audio::playSound(char sample, char loop) {

  if ((int)sample < Audio::n_samples && samples[sample].data) {
    AudioMessage msg = {.type = (char)(loop ? SOUND_CONT : SOUND_ONCE),
                        .size = samples[sample].size};
    write(Audio::writefd, &msg, sizeof(AudioMessage));
    write(Audio::writefd, samples[sample].data, samples[sample].size);
  }
}

void Audio::audioServer() {

  Audio::mixer = new AudioChannel[Audio::channels];
  bzero(Audio::mixer, Audio::channels * sizeof(AudioChannel));

  char buffer[Audio::blocksize], *a, *b, *data = NULL;

  AudioMessage msg;

  ssize_t size;
  while (true) {
    size = read(Audio::readfd, &msg, sizeof(AudioMessage));
    if (size == sizeof(AudioMessage)) {
      if (msg.type == SOUND_EXIT)
        break;
      else if (msg.type == SOUND_CLEAR) {
        for (int i = Audio::channels; i--;) {
          if (Audio::mixer[i].data) {
            delete Audio::mixer[i].data;
            Audio::mixer[i].data = NULL;
          }
        }
      } else if (msg.size) {
        data = new char[msg.size];
        size = read(Audio::readfd, data, msg.size);
        if (size == msg.size) {
          int channel = Audio::channels;
          for (int i = Audio::channels; i--;) {
            if (!Audio::mixer[i].data) {
              channel = i;
              Audio::mixer[i].size = msg.size;
              Audio::mixer[i].pos = 0;
              Audio::mixer[i].data = data;
              Audio::mixer[i].loop = msg.type == SOUND_CONT ? 1 : 0;
              break;
            }
          }

          if (channel == Audio::channels) {
            delete data;
            data = NULL;
          }
        } else {
          delete data;
          data = NULL;
        }
      }
    }

    if (Audio::audioOn) {
      bzero(buffer, Audio::blocksize);
      for (int i = Audio::channels; i--;) {
        if (!Audio::mixer[i].data)
          continue;

        if (Audio::mixer[i].pos >= Audio::mixer[i].size) {
          if (Audio::mixer[i].loop)
            Audio::mixer[i].pos = 0;
          else {
            delete Audio::mixer[i].data;
            Audio::mixer[i].data = NULL;
            continue;
          }
        }

        a = Audio::mixer[i].data + Audio::mixer[i].pos;
        Audio::mixer[i].pos += (Audio::blocksize + 1);
        b = buffer;

        int j = blocksize, diff = Audio::mixer[i].size - Audio::mixer[i].pos;
        if (diff < 0)
          j += diff;

        while (j--)
          *b++ += *a++;
      }

      write(Audio::dsp, buffer, Audio::blocksize);
    }
  }

  for (int i = Audio::channels; i--;) {
    if (Audio::mixer[i].data)
      delete Audio::mixer[i].data;
  }
  delete Audio::mixer;
  exit(0);
}
