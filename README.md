#MobileMouse FOSS server for linux

###build:
    sh builddep.sh  
    mkdir build  
    cd build   
    cmake ..   
    make  

###install:
    sudo make install  



##ORIGINAL README (Eric Lax):

About:

 This application was created by Erik Lax <erik@datahack.se>.

 http://sourceforge.net/projects/mmlinuxserver/

Build:

 sh builddep.sh
 mkdir build
 cd build
 cmake ..
 make

Install:

 sudo make install

 cp ../mmserver.conf ~/.mmserver/mmserver.conf
 chmod 0600 ~/.mmserver/mmserver.conf

 Edit ~/.mmserver/mmserver.conf accordingly.

 Test run /usr/sbin/mmserver ~/.mmserver/mmserver.conf

Autorun:

 In Gnome, go to System -> Preferences -> Startup Applications
 add a new Entry

 Name: Mobile Mouse Server
 Command: /usr/sbin/mmserver /home/<username>/.mmserver/mmserver.conf
 Comment: (empty)

 Login and logout again...

Have fun!
