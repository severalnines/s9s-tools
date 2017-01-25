%define build_timestamp %(date +"%Y%m%d")

Name: s9s-tools
Version: 0.1
Release: %{build_timestamp}%{?dist}
Summary: Severalnines ClusterControl CLI Tools

License: GPLv2+
URL: https://github.com/severalnines/s9s-tools/
Source0: https://github.com/severalnines/s9s-tools/archive/master.zip

BuildRequires: bison
BuildRequires: automake
BuildRequires: gcc-c++
BuildRequires: openssl-devel
BuildRequires: flex

%description
Severalnines ClusterControl CLI Tools

%prep
%setup -q -n s9s-tools-master

%build
./autogen.sh
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%doc README TODO COPYING ChangeLog
%{_bindir}/*
%{_libdir}/*
%{_mandir}/man1/*
%{_mandir}/man5/*

%changelog
* Wed Jan 25 2017 David Kedves <kedazo@severalnines.com> 0.1.20170125
- Initial RPM release
