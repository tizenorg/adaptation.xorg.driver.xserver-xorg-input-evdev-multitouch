#sbs-git:slp/pkgs/xorg/driver/xserver-xorg-input-evdev-multitouch xserver-xorg-input-evdev-multitouch 2.3.2 b89f300e6969a0b8cef3bbe5720ec5300baf4ad3
Name:	xorg-x11-drv-evdev-multitouch
Summary:    X.Org evdev multitouch input driver.
Version: 2.3.2
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(xproto)
BuildRequires:  pkgconfig(randrproto)
BuildRequires:  pkgconfig(inputproto)
BuildRequires:  pkgconfig(kbproto)
BuildRequires:  pkgconfig(resourceproto)
BuildRequires:  pkgconfig(xkbfile)

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
%setup -q

%build
export CFLAGS+=" -Wall -g -D_F_IGNORE_TSP_RESOLUTION_ -D_F_SUPPORT_PREFERRED_NAME_ -D_F_GESTURE_EXTENSION_ "

%autogen --disable-static
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%files
/usr/include/xorg/evdevmultitouch-properties.h
/usr/lib/pkgconfig/xorg-evdev-multitouch.pc
/usr/lib/xorg/modules/input/evdevmultitouch_drv.so
/usr/share/man/man4/evdevmultitouch.4.gz

