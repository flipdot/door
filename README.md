# door

Door control system for the flipdot hackspace. It runs on a Raspberry Pi at the
door together with a custom PCB in Raspberry Pi HAT form factor
(`pihat_door`).

The current door control system is
**[flipdot/flipdoor](https://code.flipdot.org/flipdot/flipdoor)**, a Rust
project with web and SSH interfaces that serves
[door.flipdot.space](https://door.flipdot.space). The Python implementation in
this repository is legacy and no longer maintained. The sections below
document that implementation and the hardware, which remain useful for
reference.

The system:

- continuously reads the door state (open/closed) from a switch on the door
- unlocks/locks the door on request
  - from a [web frontend](https://door.flipdot.space) (flipdot LDAP login,
    opening is restricted to flipdot members)
  - via RFID: members scan their card with a USB RFID reader
  - via MQTT close commands
- publishes the door state to `api.flipdot.org`, where it is displayed on the
  flipdot.org status page

## Repository layout

| Path | Description |
| --- | --- |
| `hardware/` | KiCad project for the `pihat_door` PCB |
| `door_firmware.X/` | Firmware for the PIC microcontroller on the HAT (MPLAB X project, XC8, PIC16LF1719) |
| `door_lib.py` | Shared Python library: serial access, door state, open/close, API update |
| `web.py` | Flask web frontend (login via LDAP, door state, open/close) |
| `mqtt2door.py` | MQTT bridge + RFID reader daemon |
| `door.py` | Small script that reads the door state and logs it to syslog |
| `door_state.sh` | Shell helper that reads the door state from the serial port |
| `acme.py`, `acme.sh/` | (legacy) TLS via Let's Encrypt/acme.sh; TLS is now handled by nginx |
| `door-master.nginx.conf` | Example nginx configuration (TLS, reverse proxy) |
| `web.service`, `mqtt2door.service` | systemd units |
| `sync.sh` | Syncs `authorized_keys` for the `opendoor` user from LDAP |
| `sync_rfid_tags.sh` | Regenerates `rfid_tags.json` (employee numbers) from LDAP |
| `config.example.py` | Configuration template (copy to `config.py`, see below) |

The `ldap_user/` and `acme.sh/` submodules provide LDAP authentication and
certificate handling respectively.

## Hardware and serial protocol

The HAT carries a PIC16LF1719 that is wired to the Raspberry Pi GPIO header.
It talks over a UART at 9600 baud 8N1 (`config.SERIAL`, default
`/dev/ttyAMA0`).

The PIC continuously samples the door switch (ADC, averaged over 30 samples)
and prints one line per batch:

```
<adc> - <ra0>\r\n
```

- `<adc>`: 10-bit average of the door switch reading; values `> 500` mean the
  door is **open**
- `<ra0>`: digital input on pin RA0

Commands (single character, no newline required):

- `1` – pulse the door open (~200 ms)
- `0` – pulse the door closed (~200 ms)
- anything else – echoed back

The PIC sends `b` at startup and `r` (followed by a UART re-init) when it hits
a UART over-error. `door_lib.get_state()` skips lines that do not start with a
number, so these are handled transparently.

## Web frontend

`web.py` is a small Flask app:

- login with flipdot LDAP credentials (`ldap_user` submodule)
- shows the current door state (10 s cache, or fake data when
  `config.fake_door` is set)
- "open door" button – only usable by flipdot members
  (`is_member` LDAP attribute)
- "close door" button

```sh
pip install -r requirements.txt
cp config.example.py config.py  # adjust DOMAIN, EMAIL, SERIAL, ...
./web.py
```

For production it runs under systemd (`web.service`) behind nginx
(`door-master.nginx.conf`), which terminates TLS.

Set `fake_door = True` in `config.py` to run the app without the hardware.

## MQTT and RFID

`mqtt2door.py` (run under systemd as `mqtt2door.service`) does three things:

1. Connects to the MQTT broker at `power.flipdot.space:1883` and closes the
   door on the `actor/door/close` topic (plus a raw ESP sensor topic).
2. Polls the USB RFID reader on `/dev/ttyUSB0`. If the scanned tag is in
   `rfid_tags.json`, the door is opened, otherwise the event is only logged.
   `sync_rfid_tags.sh` regenerates `rfid_tags.json` from the `employeeNumber`
   attributes of all LDAP members, so member cards work automatically.
3. Publishes the door state to
   `https://api.flipdot.org/sensors/door/locked/<0|1>` every 60 seconds.

## Firmware

`door_firmware.X/` is an MPLAB X project for the PIC16LF1719 on the HAT
(build with the XC8 compiler, e.g. via MPLAB X IDE). See
`hardware/pihat_door.pro` for the board design.

## Configuration

Copy `config.example.py` to `config.py` (git-ignored) and adjust:

| Key | Meaning |
| --- | --- |
| `PORT`, `HTTPS_PORT` | Ports for the Flask app |
| `SERIAL` | Serial device of the HAT, e.g. `/dev/ttyAMA0` |
| `DOMAIN`, `EMAIL` | Domain and contact for certificate issuance |
| `DEBUG` | Verbose logging |
| `STAGING` | Use the Let's Encrypt staging server |
| `fake_door` | Simulate the door state (for development) |
