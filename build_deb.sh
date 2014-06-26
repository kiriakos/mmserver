export DEBEMAIL="ssabchew at yahoo dot com"
export DEBFULLNAME="Stiliyan Sabchew"
PROJ_NAME="mmserver"
VER=$( git describe --long | cut  -c 2- | cut -d "-" -f 1 || echo 'unknown' )
cp -a ../"$PROJ_NAME" ../"$PROJ_NAME-$VER"
cd ../"$PROJ_NAME-$VER"
tar -czf ../"$PROJ_NAME-$VER.tar.gz" ../"$PROJ_NAME-$VER"
echo s|dh_make -f ../"$PROJ_NAME-$VER".tar.gz
dpkg-buildpackage -us -uc
rm -rf ../"$PROJ_NAME-$VER"
rm -rf ../"$PROJ_NAME-$VER".tar.gz
ls ../*.deb &>/dev/null && echo -e "\n\n=== this is your package:" ../*.deb ;echo
