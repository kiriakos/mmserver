#!/bin/sh

# ubuntu/debian dependencies
sudo apt-get install\
		cmake\
		g++\
		libx11-dev libxtst-dev\
		libpcre3-dev\
		libavahi-common-dev libavahi-client-dev\
		libconfig++-dev\
        libevdev-dev \
		libgtk2.0-dev \
		libxmu-dev libxt-dev

# alternative dependency list, purportedly for arch linux pacman
# pkgs="cmake gcc libx11 libxtst lib32-pcre pcre avahi libconfig gtk2"
