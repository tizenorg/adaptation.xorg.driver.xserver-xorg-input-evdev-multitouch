Name:           xorg-input-evdev-multitouch
Version:        2.3.2
Release:        3
License:        MIT
Summary:        X
Group:          System/X11
Source0:        xf86-input-evdev-multitouch-%{version}.tar.gz
Source1001:     xorg-input-evdev-multitouch.manifest
BuildRequires:  pkgconfig(inputproto)
BuildRequires:  pkgconfig(kbproto)
BuildRequires:  pkgconfig(randrproto)
BuildRequires:  pkgconfig(xkbfile)
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(xproto)

%description
X.Org X server -- evdev input multitouch driver This package provides the driver for input devices using evdev, the Linux
 kernel's event delivery mechanism.  This driver allows for multiple keyboards
 and mice to be treated as separate input devices.
 .
 More information about X.Org can be found at:
 <URL:http://www.X.org>
 <URL:http://xorg.freedesktop.org>
 <URL:http://lists.freedesktop.org/mailman/listinfo/xorg>
 .
 This package is built from the X.org xf86-input-evdev driver module.

%prep
%setup -q -n xf86-input-evdev-multitouch-%{version}

%build
cp %{SOURCE1001} .
export CFLAGS+=" -Wall -g -D_F_IGNORE_TSP_RESOLUTION_"


%autogen --disable-static
%configure --disable-static
make %{?_smp_mflags}

%install
%make_install


%files
%manifest xorg-input-evdev-multitouch.manifest
%{_includedir}/xorg/evdevmultitouch-properties.h
%{_prefix}/lib/pkgconfig/xorg-evdev-multitouch.pc
%{_prefix}/lib/xorg/modules/input/evdevmultitouch_drv.so
%{_mandir}/man4/evdevmultitouch.4.gz

