#sbs-git:slp/pkgs/xorg/driver/xserver-xorg-input-evdev-multitouch xserver-xorg-input-evdev-multitouch 2.3.2 b89f300e6969a0b8cef3bbe5720ec5300baf4ad3
Name:	xorg-x11-drv-evdev-multitouch
Summary:    X.Org evdev multitouch input driver.
Version: 2.3.16
Release:    1
VCS:        adaptation/xorg/driver/xserver-xorg-input-evdev-multitouch#REBASE-12-ge51bbb2836008916253c638ba0dc81875d6571da
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
 This package is built from the evdev multitouch driver module.

%package devel
Summary:    Development files for xorg evdev multitouch driver
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains xorg evdev multitouch development files

%prep
%setup -q

%build
export CFLAGS+=" -Wall -g -D_F_SUPPORT_PREFERRED_NAME_ -D_F_GESTURE_EXTENSION_ "
%if "%{?tizen_profile_name}" == "mobile"
export CFLAGS+=" -D_ENV_MOBILE_"
%else
%if "%{?tizen_profile_name}" == "wearable"
export CFLAGS+=" -D_F_SUPPORT_ROTATION_ANGLE_ -D_F_INVERT_XY_FOR_MULTITOUCH_ -D_F_SWAP_AXES_FOR_MULTITOUCH_ -D_ENV_WEARABLE_ "
%endif
%endif

%autogen -i -v -f
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af COPYING %{buildroot}/usr/share/license/%{name}
%make_install


%files
%defattr(-,root,root,-)
/usr/lib/xorg/modules/input/evdevmultitouch_drv.so
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
/usr/include/xorg/evdevmultitouch-properties.h
/usr/lib/pkgconfig/xorg-evdev-multitouch.pc
/usr/share/man/man4/evdevmultitouch.4.gz
