# Alerter Daemon

`Alerter`'s network daemon that takes care of distributing the visual coffee/tea/lunch break 'alerts' and chat messages among its clients.

## Usage

The daemon is compiled with:

```shell
make
```

This results in a binary executable called `dalert`, which can be invoked as:

```shell
./dalert &
```

Optionally, a port number can be given as a command line argument. The port defaults to `7777`.

```shell
./dalert 7676 &
```

## Logging

The `Alerter` daemon logs to a file called `dalert.log`, located in the directory the daemon was started from.

## Controlling the Daemon

Interaction with the `Alerter` daemon is possible by initiating a `telnet` connection to it and entering `LOGIN`, followed by providing `admin` for the pass phrase.

```shell
$ telnet 127.0.0.1 7777
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
LOGIN
Enter pass phrase : admin
Pass phrase accepted
Welcome to DALERT
This is version 1.5

> 
```

The table below lists the available commands and their function.

|command|function|
--------|---------
|HELP|lists available commands|
|LOGOUT|logout|
|QUIT|quit the daemon|
|WHO|provides a list of logged-in users|

## Notes

1. Messages will scroll by as they are being relayed by the daemon.

## BSD-3 License

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
