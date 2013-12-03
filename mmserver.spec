%global commit 5881a43e9640bebf08bdfe40ce54d4531c07ced2
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:           mmserver
Version:        1.4.0
Release:        1.SS%{?dist}
Summary:        Modified Mobile Mouse Server for Linux. Intended for (but not limited to) use with Raspbian & Raspberry Pi.
Group:          Applications/System
License:        GPL2
# https://github.com/anoved/mmserver/archive/v1.4.0.tar.gz
Source0:        https://github.com/anoved/mmserver/archive/%{commit}/%{name}-%{version}-%{shortcommit}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  rpmdevtools
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
%setup -qn %{name}-%{commit}

%build
mkdir build
cd build
cmake ..
make %{?_smp_mflags}

%install
cd build
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

# Do not wanna autostart the service
rm -rf $RPM_BUILD_ROOT/usr/share/gnome
cd ..
%__install -D -m0644 ./icons/mm-connected.svg $RPM_BUILD_ROOT/usr/share/mmserver/icons/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc LICENSE README.md
%{_sbindir}/mmserver
%{_datadir}/applications/mmserver.desktop
%{_datadir}/mmserver/*

%changelog
* Mon Dec 2 2013 Stiliyan Sabchew <ssabchew at yahoo dot com> - 0.4.0-1
- Removed pathcing config file.
* Sat Nov 30 2013 Stiliyan Sabchew <ssabchew at yahoo dot com> - 0.4.0-0
- Initial packing.
- Change debug to false.
- Tested with version 3.0.2 of Mobile Mouse (free version).
