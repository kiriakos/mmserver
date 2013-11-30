Name:           mmserver
Version:        1.4.0
Release:        0.SS%{?dist}
Summary:        Modified Mobile Mouse Server for Linux. Intended for (but not limited to) use with Raspbian & Raspberry Pi.
Group:          Applications/System
License:        GPL2
# Actual link - should be renamed to %{name}-%{version}.tar.gz
# TODO: Check how to adjust this URLs from GITHUB :(, for autmated builds with spectool.
# https://github.com/anoved/mmserver/archive/v1.4.0.tar.gz
Source0:        https://github.com/anoved/mmserver/archive/%{name}-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cmake, gcc-c++
BuildRequires:  libX11-devel libXtst-devel
BuildRequires:  pcre-devel, avahi-devel
BuildRequires:  libconfig-devel, gtk2-devel

%description
Modified Mobile Mouse Server for Linux. Intended for (but not limited to) use with Raspbian & Raspberry Pi.
Use your iOS or Android device as a network mouse and keyboard for your Linux computer using Mobile Mouse and this software, 
which is a fork of Mobile Mouse Server for Linux by Erik Lax.
This fork by Jim DeVona is based on an earlier fork by Kiriakos Krastillos.

%prep
%setup -q

%build
mkdir build
cd build
cmake ..
make %{?_smp_mflags}

%install
## TODO: Provide patches instead of using sed
sed -i 's|debug: true|debug: false|' mmserver.conf
sed -i '/## License/ i\
### Sabchew ###\nRecantly ( 2013-11-30 ) tested with version 3.0.2 of Mobile Mouse ( free version ) on iPhone iOS 6.1.3.\n### Sabchew ###\n' README.md

cd build
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

# Do not wanna autostart the service
rm -rf $RPM_BUILD_ROOT/usr/share/gnome
cd ..
cp ./icons/mm-connected.svg $RPM_BUILD_ROOT/usr/share/mmserver/icons/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc LICENSE README.md
%{_sbindir}/mmserver
%{_datadir}/applications/mmserver.desktop
%{_datadir}/mmserver/*

%changelog
* Sat Nov 30 2013 Stiliyan Sabchew <ssabchew at yahoo dot com> - 0.4.0
- Initial packing.
- Change debug to false.
- Tested with version 3.0.2 of Mobile Mouse (free version).
