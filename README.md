# About

## What is it?

## 
This is not a smart home devices controller.

i.e

If you want a light to turn on when the PC is set to "Do not Disturb" mode, this app will not directly control the light. This app exposes the "Do not distrurb" mode to your controller (Home Assistant) so that you can create an automation there.

# Current State

This is at a level where if you're ok compiling things from source and meddling with config files with no other hints it's usable. It's not a good user-facing product yet.

# Setup

mkdir build
cd build
cmake ..
make
make install

# MQTT

In home assistant MQTT server must be enabled.
See https://www.home-assistant.io/integrations/mqtt/

The following configuration needs to be placed in .config/kde-harc

```
 [general]
 host=some.host
 port=1883
 user=myUsername
 password=myPassword
 ```

On the home assistant side everything should then work out-the-box with MQTT discovery.


# Supported Features (so far)

 - User activity (binary sensor)
 - Locked state (switch)
 - Suspend (button)
 - Accent Colour (sensor)
 - Arbitrary Scripts (buttons)
 - Shortcuts (device_trigger)
 - Nightmode Inhibition (binary sensor)

# Additional Config

```
[general]
host=some.host
port=1883
user=myUsername
password=myPassword

[Scripts][myScript1]
Name=Launch chrome
Exec=google-chrome

[Scripts][myScript2]
...

[Shortcuts][myShortcut1]
Name=Do a thing


```
 
 
# Related Links

https://github.com/muniter/halinuxcompanion

I found this long after starting this project. They make use of the native mobile_app integration which is nice, and allows for some two-way integration if we wanted to get applets and whatnot. But we do some core components a bit better without polling so it's more reactive.

