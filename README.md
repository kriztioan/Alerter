# Alerter

`Alerter` is a networking application that distributes visually pleasing coffee/tea/lunch break 'alerts' among clients connected to a common server using a theme-able,  windowless interface with animations, sounds, and chat functionality.

`Alerter` is written in C/C++ and relies on `X11`, `X11's Shape Extension`, `X11's Back-buffer Extension`, `X11 Xpm support`, `XToolkit Extension`, `OpenMotif`, and `libpng`.

`Alerter` will run on Linux and MacOS, where the former requires `OSS` support for sound, and the latter the [XQuartz](https://www.xquartz.org) X11  window manager installed.

## Usage

The `Alerter` client program is compiled with:

```shell
make
```

This results in a binary executable called `alert`, which can be invoked as follows:

```shell
./alert &
```

The `Alerter` client needs to connect a daemon to relay alerts and messages. The daemon is compiled with:

```shell
make daemon
```

This results in a binary executable called `dalert`, which can be invoked as:

```shell
./dalert &
```

See [daemon/README.md](daemon/README.md) for additional details on configuring and interacting with the daemon.

## Logging

The `Alerter` client's log and configuration file are written to the user's home directory as `alert.log` and `.alert`, respectively.

## Interacting with the Client

`Alerter` recognizes the following keys.

|key|function|
--------|---------
|`q`|quit|

The recognized mouse buttons and their function are listed in the table below.

|button|function
-------|--------
|left|alert|
|middle|open chat window|
|right|open configuration window|

## Theming

`Alerter` uses a straight-forward theming system that relies on a simple text (`.theme`) file and PNG images. A dozen-or-so themes are provided in the [themes](themes/)-directory, which can be the starting point for one's own creations.

## Notes

1. On MacOS XCode and the developer tools needs to be installed.
2. On Ubuntu the `osspd-alsa` package is needed for sound.
3. Both Linux and MacOS require `libpng`.

## BSD-3 License

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
