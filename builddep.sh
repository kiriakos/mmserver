#!/bin/sh

# the following packages are required to build on arch linux
# gcc and gcc-multilib should be compatible so if you have 
pkgs="cmake gcc libx11 libxtst lib32-pcre pcre avahi libconfig gtk2"
pkgc=0

echo -n "Dependencies:"
for dp in $pkgs
do
    [ $pkgc -gt 0 ] && echo -n ","
    echo -n " $dp"
    pkgc=$(( $pkgc + 1 ))
done

echo
echo "
gcc and gcc-multilib should be compatible so if you have one or the other do not replace it.
"

sudo pacman -S --needed $pkgs
