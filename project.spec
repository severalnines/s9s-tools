%define build_timestamp %(date +"%Y%m%d")

Name: s9s-tools
Version: 1.6
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
rm -rf $RPM_BUILD_ROOT/usr/include/s9s

%files
%doc README TODO COPYING ChangeLog
%{_bindir}/*
%{_libdir}/*
%{_mandir}/man1/*
%{_mandir}/man5/*
%{_sysconfdir}/bash_completion.d/s9s_completion

%changelog
* Fri Apr 20 2018 David Kedves <kedazo@severalnines.com> 1.6.20180420
- New clustercontrol release, 1.6.0
* Thu Mar  2 2018 David Kedves <kedazo@severalnines.com> 1.5.20180302
- PostgreSQL cluster creation
* Thu Mar  1 2018 David Kedves <kedazo@severalnines.com> 1.5.20180301
- New patch release
* Fri Dec  8 2017 David Kedves <kedazo@severalnines.com> 1.5.20171208
- A new release with MongoDB cluster creation possibility
* Mon Nov 13 2017 David Kedves <kedazo@severalnines.com> 1.5.20171113
- Releasing with clustercontrol-controller 1.5.0
* Wed Oct 18 2017 David Kedves <kedazo@severalnines.com> 1.5.20171018
- Version up, to match with the upcoming clustercontrol release
* Wed Oct  3 2017 David Kedves <kedazo@severalnines.com> 0.1.20171003
- Fixing RPM building issue
* Wed Jan 25 2017 David Kedves <kedazo@severalnines.com> 0.1.20170125
- Initial RPM release
