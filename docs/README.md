# Electronics for Eaglemoss Models

[![Build Status](https://travis-ci.org/pages-themes/minimal.svg?branch=master)](https://travis-ci.org/pages-themes/minimal) 

## Our principles in Extension boards design

Our extension boards are designed with the following rules:

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

Resources:
- [manuel utilisateur / user manual](https://github.com/henrio-net/dkmod-delorean/blob/master/docs/Delorean%20RF%20Controller%20-%20Manuel%20Utilisateur.pdf)
- [guide pour utilisateur Arduino](https://github.com/henrio-net/dkmod-delorean/blob/master/docs/Delorean%20RF%20Controller%20-%20Utilisateur%20Arduino.pdf)
- [code source](https://github.com/henrio-net/dkmod-delorean/blob/master/code)



### Delorean RF Player

This board is an extension to the RF Controller that integrates a MP3 player.
Being so enthousiastic about embedding music player in the model, a bunch of functionalities have been developped in the board software. Today, the functionalities are:

1. Command the 6 buttons separately
2. Powering on/off all the devices
3. Play music from MP3 playlist
4. Navigate in MP3 playlist
5. Advertise randomly from ADVERT playlist (so cool to play your favourite sentences from the movie)
6. Record / Play a sequence of commands timely on a music track
7. Change volume
8. Change equalizer


This board is like the ultimate one !
But yeah, there's a drawback: Since the original 3xAA battery leads to ridiculous autonomy, this extension board requires a well regulated 5V 2A minimum power supply to work correctly. This is mainly due to the onboard 3w audio amplifier.

Ressources:
- [user manual ENG](https://github.com/henrio-net/dkmod-delorean/blob/master/docs/Delorean%20RF%20Player%20-%20User%20Manual%20v1.pdf)
- [manuel utilisateur FR](https://github.com/henrio-net/dkmod-delorean/blob/master/docs/Delorean%20RF%20Player%20-%20Manuel%20Utilisateur%20v1.pdf)
- [guide pour Mac OS](https://github.com/henrio-net/dkmod-delorean/blob/master/docs/CarteSD-sous-MacOS.pdf)
- [code source](https://github.com/henrio-net/dkmod-delorean/blob/master/code)


# About dkMOD
We are BTTF fan and technology enthousiast and mixing those two passions is really great.
Hopefully we will find the time to work again on such projects.
For any information or any personnal request, feel free to contact us !

Facebook page: [facebook.com/dkmod.officiel](facebook.me/dkmod.officiel)

Contact us: [m.me/dkmod.officiel](m.me/dkmod.officiel)
