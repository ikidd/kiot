# About

# Setup

cmake
make
make install

as usual

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

# Supported Features

 - Idle (binary sensor)
 - Locked state (switch)
 - Suspend (button)

## TODO

 - Accent colour has a Light
 - Nightlight state / inhibition
 - Battery?
 - Microphone in use binary sensor
 - ??
 - Setup KCM

# Architecture

core.cpp has types that match the low level MQTT types supported in HA. Switch/Sensor/Etc.
components.cpp is a set of "plugins" that instantiate one or more Entities and provide the system integration

# Alternatives

https://github.com/muniter/halinuxcompanion

I found this after starting this project. They make use of the native mobile_app integration which is nice, and allows for some two-way integration if we wanted to get applets and whatnot. But we do some core components a bit better without polling so it's more reactive.

Could be worth joining forces or copying code depending on how much we lean towards better KDE integration vs global compatibility.
