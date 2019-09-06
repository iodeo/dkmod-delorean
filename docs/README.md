# Delorean Mods on Electronic devices control

[![Build Status](https://travis-ci.org/pages-themes/minimal.svg?branch=master)](https://travis-ci.org/pages-themes/minimal) [![Gem Version](https://badge.fury.io/rb/jekyll-theme-minimal.svg)](https://badge.fury.io/rb/jekyll-theme-minimal)

## Our principles in Extension boards design

Our extension boards are designed to respect the following rules:

1. Fit in existing model perfectly

2. Be plug and play

3. Be end-user friendly (not only for programmer)

4. Be open source and customisable

## Our boards

We are starting with boards which enable Delorean remote control

### Delorean RF Controller

This board enables to remote control the 6 buttons of the model with a nice remote.
The physical buttons can still be used in parallel.

Some optional extras may be added such as:
- Adding a command to control the 6 buttons in a row
- Adding a sequencer


### Delorean RF Player

This board is an extension to the RF Controller that integrates a MP3 player.
Being so enthousiastic about embedding music player in the model, a bunch of functionalities have been developped in the board software. Today, the functionalities are:

1. Play music from a playlist
2. Navigate in the playlist
3. Change volume
4. Change equalizer
5. Advertise randomly from another playlist (so cool to play your favourite sentences from the movie)
6. Record / Play a sequence of commands timely on a music track
7. Powering on/off all the devices

This board is like the ultimate one !
But yeah, there's a drawback: Since the original 3xAA battery leads to ridiculous autonomy, this extension board requires a 5V 2A minimum power supply to work correctly. This is mainly due to the onboard 3w audio amplifier.

Ressources:
- [user manual ENG](https://github.com/henrio-net/dkmod-delorean/raw/master/docs/Delorean%20RF%20Player%20-%20user%20manual%20-%20v1.pdf)
- [user manual FR](https://github.com/henrio-net/dkmod-delorean/raw/master/docs/Delorean%20RF%20Player%20-%20manuel%20utilisateur%20-%20v1.pdf)
- [code source](code/HOLD)


# About dkMOD
We are BTTF fan and technology enthousiast and mixing those two passions has been really great.
Hopefully we will find the time to work again on such projects in the future.
For any information or any personnal request, feel free to contact us !

Contact: [dkmod@techie.com](mailto:dkmod@techie.com)

