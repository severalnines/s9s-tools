%define build_timestamp %(date +"%Y%m%d")

Name: s9s-tools
Version: 1.9
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
BuildRequires: gdb

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
%{_mandir}/man3/*
%{_mandir}/man5/*
%{_sysconfdir}/bash_completion.d/s9s_completion

%changelog
* Tue Jun 29 2021 David Kedves <kedazo@severalnines.com> 1.9.20210629
- New release
* Mon May 17 2021 David Kedves <kedazo@severalnines.com> 1.9.20210517
- New release
* Wed Apr 07 2021 David Kedves <kedazo@severalnines.com> 1.9.20210407
- nightly build for inside testing
* Tue Jan 26 2021 David Kedves <kedazo@severalnines.com> 1.8.20210126
- Register (ProxySQL,Keepalived,HAProxy) host fixes
- Other small fixes + lots of tests updated and added
* Fri Nov 13 2020 David Kedves <kedazo@severalnines.com> 1.8.20201113
- Added s9s cluster --sync option (to sync backend with frontend database)
- Percona Backup for MongoDB support
* Wed Oct 28 2020 David Kedves <kedazo@severalnines.com> 1.8.20201028
- New internal release
* Tue Oct 13 2020 David Kedves <kedazo@severalnines.com> 1.8.20201013
- New internal release
* Fri Oct  2 2020 David Kedves <kedazo@severalnines.com> 1.8.20201002
- A new public release
* Mon Sep 21 2020 David Kedves <kedazo@severalnines.com> 1.8.20200921
- A new public release
* Fri Sep 18 2020 David Kedves <kedazo@severalnines.com> 1.8.20200918
- A new public release
* Thu Sep  3 2020 David Kedves <kedazo@severalnines.com> 1.8.20200903
- A new public release
* Mon Aug 24 2020 David Kedves <kedazo@severalnines.com> 1.8.20200824
- Creating a pre-release build
* Thu Aug  4 2020 David Kedves <kedazo@severalnines.com> 1.7.20200804
- Creating a pre-release build
* Thu May 28 2020 David Kedves <kedazo@severalnines.com> 1.7.20200528
- Creating a new build
* Tue May 26 2020 David Kedves <kedazo@severalnines.com> 1.7.20200526
- Creating a new build
* Wed May  6 2020 David Kedves <kedazo@severalnines.com> 1.7.20200506
- Version bump
* Mon Apr 20 2020 David Kedves <kedazo@severalnines.com> 1.7.20200420
- Version bump
* Tue Apr 14 2020 David Kedves <kedazo@severalnines.com> 1.7.20200414
- Bersion bump
* Tue Mar 10 2020 David Kedves <kedazo@severalnines.com> 1.7.20200310
- New pre-release testing build
* Fri Feb 21 2020 David Kedves <kedazo@severalnines.com> 1.7.20200228
- Pre-release testing build
* Fri Dec 13 2019 David Kedves <kedazo@severalnines.com> 1.7.20191213
- Release for the upcoming 1.7.5 release
* Thu Oct 24 2019 David Kedves <kedazo@severalnines.com> 1.7.20191024
- Release for the upcoming 1.7.4 release
* Thu Oct 17 2019 David Kedves <kedazo@severalnines.com> 1.7.20191017
- New release for clustercontrol 1.7.3 release (and for the next soon)
* Fri Jul 12 2019 David Kedves <kedazo@severalnines.com> 1.7.20190712
- New release for clustercontrol 1.7.3 release
* Tue May 14 2019 David Kedves <kedazo@severalnines.com> 1.7.20190514
- Added the --config-template and --no-install options.
* Tue Apr 30 2019 David Kedves <kedazo@severalnines.com> 1.7.20190430
- New patch release
* Sat Apr  6 2019 David Kedves <kedazo@severalnines.com> 1.7.20190406
- New release for clustercontrol 1.7.2
* Fri Jan  4 2019 David Kedves <kedazo@severalnines.com> 1.7.20190104
- Just a new build with minor bugfixes
* Mon Dec 24 2018 David Kedves <kedazo@severalnines.com> 1.7.20181224
- Build for new clustercontrol release (1.7.1)
* Tue Nov 13 2018 David Kedves <kedazo@severalnines.com> 1.7.20181113
- Bugfixes
* Thu Nov  8 2018 David Kedves <kedazo@severalnines.com> 1.7.20181108
- Bugfixes
* Mon Sep 24 2018 David Kedves <kedazo@severalnines.com> 1.7.20180924
- New clustercontrol release
* Fri Jun  8 2018 David Kedves <kedazo@severalnines.com> 1.6.20180608
- New patch release
* Tue Jun  5 2018 David Kedves <kedazo@severalnines.com> 1.6.20180605
- New patch release
* Thu May  3 2018 David Kedves <kedazo@severalnines.com> 1.6.20180503
- New patch release
* Tue May  1 2018 David Kedves <kedazo@severalnines.com> 1.6.20180501
- New patch release
* Fri Apr 20 2018 David Kedves <kedazo@severalnines.com> 1.6.20180420
- New clustercontrol release, 1.6.0
* Fri Mar  2 2018 David Kedves <kedazo@severalnines.com> 1.5.20180302
- PostgreSQL cluster creation
* Thu Mar  1 2018 David Kedves <kedazo@severalnines.com> 1.5.20180301
- New patch release
* Fri Dec  8 2017 David Kedves <kedazo@severalnines.com> 1.5.20171208
- A new release with MongoDB cluster creation possibility
* Mon Nov 13 2017 David Kedves <kedazo@severalnines.com> 1.5.20171113
- Releasing with clustercontrol-controller 1.5.0
* Wed Oct 18 2017 David Kedves <kedazo@severalnines.com> 1.5.20171018
- Version up, to match with the upcoming clustercontrol release
* Tue Oct  3 2017 David Kedves <kedazo@severalnines.com> 0.1.20171003
- Fixing RPM building issue
* Wed Jan 25 2017 David Kedves <kedazo@severalnines.com> 0.1.20170125
- Initial RPM release
