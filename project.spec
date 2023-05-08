%define build_timestamp %(date +"%Y%m%d")

Name: s9s-tools
Version: 1.9
Release: %{build_timestamp}%{?dist}
Summary: Severalnines ClusterControl CLI Tools

License: GPL-2.0-or-later
Group: Development/Tools/Navigators
URL: https://github.com/severalnines/s9s-tools/
Source0: https://github.com/severalnines/s9s-tools/archive/master.zip

BuildRequires: bison
BuildRequires: unzip
BuildRequires: automake
BuildRequires: gcc-c++
BuildRequires: openssl-devel
BuildRequires: flex
BuildRequires: gdb
BuildRequires: sed

%description
Severalnines ClusterControl CLI Tools

%prep
%setup -q -n s9s-tools-master

%build
# kill weirdo failures on redhat/suse systems
sed -e 's/-Werror//g' -i libs9s/Makefile.am
export EXTRAFLAGS="-Wno-error=odr"
if LC_ALL='C' g++ -Wodr 2>&1 | grep 'unrecognized.*Wodr'; then
    export EXTRAFLAGS=""
fi
CFLAGS="${CFLAGS} ${EXTRAFLAGS}" CXXFLAGS="${CXXFLAGS} ${EXTRAFLAGS}" ./autogen.sh --with-no-tests --with-no-rpath --libdir=%{_libdir}
%configure
CFLAGS="${CFLAGS} ${EXTRAFLAGS}" CXXFLAGS="${CXXFLAGS} ${EXTRAFLAGS}" ./configure --with-no-tests --with-no-rpath --libdir=%{_libdir}
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
# remove development stuff
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.a
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
* Mon May  8 2023 Severalnines <support@severalnines.com> 1.9.2023050817
- Release 1.9.2023050817.
* Thu Apr 13 2023 Severalnines <support@severalnines.com> 1.9.2023041323
- Release 1.9.2023041323.
* Wed Apr 12 2023 Severalnines <support@severalnines.com> 1.9.2023020120
- New release with compilation fixes for OpenSUSE
* Wed Mar  1 2023 Severalnines <support@severalnines.com> 1.9.2023030117
- Release 1.9.2023030117.
* Mon Feb 27 2023 Severalnines <support@severalnines.com> 1.9.2023022717
- Release 1.9.2023022717.
* Mon Jan 23 2023 Severalnines <support@severalnines.com> 1.9.2023012301
- Add initial support of CentOS9/RockyLinux9 & AlmaLinux9
* Mon Jan  9 2023 Severalnines <support@severalnines.com> 1.9.2023010901
- s9s job: scheduled jobs can be enabled/disabled (--enable/--disable)
* Wed Dec  7 2022 Severalnines <support@severalnines.com> 1.9.2022120701
- Implemented local repo options for cluster
* Fri Nov  3 2022 Severalnines <support@severalnines.com> 1.9.2022110415
- New build
* Wed Oct 19 2022 Severalnines <support@severalnines.com> 1.9.2022101916
- Release 1.9.2022101916.
* Thu Aug 11 2022 Severalnines <support@severalnines.com> 1.9.2022081117
- Release 1.9.2022081117.
* Thu Aug 11 2022 Severalnines <support@severalnines.com> 1.9.2022081116
- Release 1.9.2022081116.
* Thu Aug 11 2022 Severalnines <support@severalnines.com> 1.9.2022081116
- Release 1.9.2022081116.
* Thu Aug 11 2022 Severalnines <support@severalnines.com> 1.9.2022081116
- Release 1.9.2022081116.
* Mon Jul 18 2022 Severalnines <support@severalnines.com> 1.9.2022071819
- Release 1.9.2022071819.
* Thu Jun 23 2022 Severalnines <support@severalnines.com> 1.9.2022062321
- Release 1.9.2022062321.
* Fri Jun 17 2022 David Kedves <kedazo@severalnines.com> 1.9.2022061700
- Some more packaging fixes.
* Wed Jun 15 2022 Severalnines <support@severalnines.com> 1.9.2022061515
- Release 1.9.2022061515.
* Tue Jun 14 2022 David Kedves <kedazo@severalnines.com> 1.9.2022061400
- Prepared some packaging fixes.
* Tue Jun  7 2022 Severalnines <support@severalnines.com> 1.9.2022060720
- Release 1.9.2022060720.
* Wed May 11 2022 David Kedves <kedazo@severalnines.com> 1.9.2022051109
- Fixed the version reporting.
* Wed May 11 2022 Severalnines <support@severalnines.com> 1.9.2022051102
- Release 1.9.2022051102.
* Mon May  2 2022 Severalnines <support@severalnines.com> 1.9.2022050221
- Release 1.9.2022050221.
* Mon May  2 2022 Severalnines <support@severalnines.com> 1.9.2022050221
- Release 1.9.2022050221.
* Tue Apr 12 2022 David Kedves <kedazo@severalnines.com> 1.9.20220412
- new build
* Thu Mar 24 2022 David Kedves <kedazo@severalnines.com> 1.9.20220324
- A new build for upcoming release
* Tue Feb 15 2022 David Kedves <kedazo@severalnines.com> 1.9.20220215
- A new build for internal testing
* Thu Feb 03 2022 David Kedves <kedazo@severalnines.com> 1.9.20220203
- A new build for internal testing
* Wed Jan 26 2022 David Kedves <kedazo@severalnines.com> 1.9.20220126
- A new build with some doc updates 
* Fri Jan 14 2022 David Kedves <kedazo@severalnines.com> 1.9.20220114
- New release along with ClusterControl 1.9.2 
* Wed Nov 03 2021 David Kedves <kedazo@severalnines.com> 1.9.20211103
- Pushing out a new build
* Mon Sep 27 2021 David Kedves <kedazo@severalnines.com> 1.9.20210927
- New build for internal testing
* Wed Sep 08 2021 David Kedves <kedazo@severalnines.com> 1.9.20210908
- Added register redis cluster support
* Tue Jun 29 2021 David Kedves <kedazo@severalnines.com> 1.9.20210629
- New release
* Mon Jun 07 2021 David Kedves <kedazo@severalnines.com> 1.9.20210607
- New build (expired OBS keys, meh...)
* Tue Jun 01 2021 David Kedves <kedazo@severalnines.com> 1.9.20210601
- New build
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
