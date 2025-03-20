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
* Thu Mar 20 2025 Severalnines <support@severalnines.com> 1.9.2025032022
- Testing release 1.9.2025032022.
* Thu Mar 20 2025 Severalnines <support@severalnines.com> 1.9.2025032018
- Testing release 1.9.2025032018.
* Fri Mar 14 2025 Severalnines <support@severalnines.com> 1.9.2025031419
- Testing release 1.9.2025031419.
* Thu Feb 20 2025 Severalnines <support@severalnines.com> 1.9.2025022018
- Release 1.9.2025022018.
* Thu Feb 20 2025 Severalnines <support@severalnines.com> 1.9.2025022018
- Release 1.9.2025022018.
* Fri Feb 14 2025 Severalnines <support@severalnines.com> 1.9.2025021417
- Testing release 1.9.2025021417.
* Fri Feb 14 2025 Severalnines <support@severalnines.com> 1.9.2025021417
- Testing release 1.9.2025021417.
* Mon Feb 10 2025 Severalnines <support@severalnines.com> 1.9.2025021017
- Testing release 1.9.2025021017.
* Fri Jan 31 2025 Severalnines <support@severalnines.com> 1.9.2025013116
- Testing release 1.9.2025013116.
* Thu Jan  9 2025 Severalnines <support@severalnines.com> 1.9.2025010919
- Testing release 1.9.2025010919.
* Tue Dec 17 2024 Severalnines <support@severalnines.com> 1.9.2024121720
- Testing release 1.9.2024121720.
* Fri Dec 13 2024 Severalnines <support@severalnines.com> 1.9.2024121320
- Testing release 1.9.2024121320.
* Wed Dec 11 2024 Severalnines <support@severalnines.com> 1.9.2024121119
- Testing release 1.9.2024121119.
* Thu Nov 28 2024 Severalnines <support@severalnines.com> 1.9.2024112821
- Testing release 1.9.2024112821.
* Mon Nov 25 2024 Severalnines <support@severalnines.com> 1.9.2024112518
- Release 1.9.2024112518.
* Mon Nov 25 2024 Severalnines <support@severalnines.com> 1.9.2024112518
- Release 1.9.2024112518.
* Wed Nov 20 2024 Severalnines <support@severalnines.com> 1.9.2024112020
- Testing release 1.9.2024112020.
* Wed Nov 20 2024 Severalnines <support@severalnines.com> 1.9.2024112020
- Testing release 1.9.2024112020.
* Tue Nov 19 2024 Severalnines <support@severalnines.com> 1.9.2024111919
- Testing release 1.9.2024111919.
* Tue Nov 19 2024 Severalnines <support@severalnines.com> 1.9.2024111904
- Testing release 1.9.2024111904.
* Wed Nov  6 2024 Severalnines <support@severalnines.com> 1.9.2024110618
- Testing release 1.9.2024110618.
* Wed Nov  6 2024 Severalnines <support@severalnines.com> 1.9.2024110617
- Testing release 1.9.2024110617.
* Thu Oct 10 2024 Severalnines <support@severalnines.com> 1.9.2024101022
- Testing release 1.9.2024101022.
* Wed Oct  2 2024 Severalnines <support@severalnines.com> 1.9.2024100217
- Release 1.9.2024100217.
* Wed Oct  2 2024 Severalnines <support@severalnines.com> 1.9.2024100217
- Release 1.9.2024100217.
* Mon Sep 30 2024 Severalnines <support@severalnines.com> 1.9.2024093022
- Testing release 1.9.2024093022.
* Mon Sep 30 2024 Severalnines <support@severalnines.com> 1.9.2024093018
- Testing release 1.9.2024093018.
* Thu Sep 26 2024 Severalnines <support@severalnines.com> 1.9.2024092619
- Testing release 1.9.2024092619.
* Tue Sep 17 2024 Severalnines <support@severalnines.com> 1.9.2024091721
- Testing release 1.9.2024091721.
* Wed Aug 28 2024 Severalnines <support@severalnines.com> 1.9.2024082802
- Release 1.9.2024082802.
* Wed Aug 28 2024 Severalnines <support@severalnines.com> 1.9.2024082802
- Release 1.9.2024082802.
* Tue Aug 20 2024 Severalnines <support@severalnines.com> 1.9.2024082016
- Testing release 1.9.2024082016.
* Thu Aug 15 2024 Severalnines <support@severalnines.com> 1.9.2024081501
- Testing release 1.9.2024081501.
* Thu Aug  1 2024 Severalnines <support@severalnines.com> 1.9.2024080102
- Testing release 1.9.2024080102.
* Thu Jul 18 2024 Severalnines <support@severalnines.com> 1.9.2024071816
- Testing release 1.9.2024071816.
* Thu Jul 18 2024 Severalnines <support@severalnines.com> 1.9.2024071802
- Testing release 1.9.2024071802.
* Tue Jul  9 2024 Severalnines <support@severalnines.com> 1.9.2024070920
- Testing release 1.9.2024070920.
* Thu Jul  4 2024 Severalnines <support@severalnines.com> 1.9.2024070420
- Testing release 1.9.2024070420.
* Wed Jul  3 2024 Severalnines <support@severalnines.com> 1.9.2024070318
- Testing release 1.9.2024070318.
* Mon Jun 24 2024 Severalnines <support@severalnines.com> 1.9.2024062410
- Release 1.9.2024062410.
* Mon Jun 24 2024 Severalnines <support@severalnines.com> 1.9.2024062410
- Release 1.9.2024062410.
* Fri May 17 2024 Severalnines <support@severalnines.com> 1.9.2024051718
- Testing release 1.9.2024051718.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051507
- Release 1.9.2024051507.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051507
- Release 1.9.2024051507.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051500
- Release 1.9.2024051500.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051500
- Release 1.9.2024051500.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051500
- Release 1.9.2024051500.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051500
- Release 1.9.2024051500.
* Wed May 15 2024 Severalnines <support@severalnines.com> 1.9.2024051500
- Testing release 1.9.2024051500.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051423
- Release 1.9.2024051423.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051423
- Release 1.9.2024051423.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051423
- Release 1.9.2024051423.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051423
- Release 1.9.2024051423.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051423
- Testing release 1.9.2024051423.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051420
- Release 1.9.2024051420.
* Tue May 14 2024 Severalnines <support@severalnines.com> 1.9.2024051420
- Release 1.9.2024051420.
* Thu Apr 18 2024 Severalnines <support@severalnines.com> 1.9.2024041800
- Testing release 1.9.2024041800.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040519
- Testing release 1.9.2024040519.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040519
- Testing release 1.9.2024040519.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040518
- Testing release 1.9.2024040518.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040518
- Release 1.9.2024040518.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040518
- Release 1.9.2024040518.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040518
- Testing release 1.9.2024040518.
* Fri Apr  5 2024 Severalnines <support@severalnines.com> 1.9.2024040517
- Testing release 1.9.2024040517.
* Thu Mar 21 2024 Severalnines <support@severalnines.com> 1.9.2024032117
- Testing release 1.9.2024032117.
* Tue Mar 19 2024 Severalnines <support@severalnines.com> 1.9.2024031917
- Testing release 1.9.2024031917.
* Tue Mar 19 2024 Severalnines <support@severalnines.com> 1.9.2024031917
- Testing release 1.9.2024031917.
* Thu Mar 14 2024 Severalnines <support@severalnines.com> 1.9.2024031417
- Testing release 1.9.2024031417.
* Wed Mar 13 2024 Severalnines <support@severalnines.com> 1.9.2024031322
- Testing release 1.9.2024031322.
* Tue Mar  5 2024 Severalnines <support@severalnines.com> 1.9.2024030519
- Testing release 1.9.2024030519.
* Mon Feb 26 2024 Severalnines <support@severalnines.com> 1.9.2024022611
- Release 1.9.2024022611.
* Mon Feb 26 2024 Severalnines <support@severalnines.com> 1.9.2024022611
- Release 1.9.2024022611.
* Thu Nov 30 2023 Severalnines <support@severalnines.com> 1.9.2023113021
- Release 1.9.2023113021.
* Wed Nov 29 2023 Severalnines <support@severalnines.com> 1.9.2023112922
- Release 1.9.2023112922.
* Wed Nov 29 2023 Severalnines <support@severalnines.com> 1.9.2023112913
- Release 1.9.2023112913.
* Tue Nov 28 2023 Severalnines <support@severalnines.com> 1.9.2023112821
- Release 1.9.2023112821.
* Thu Nov 23 2023 Severalnines <support@severalnines.com> 1.9.2023112301
- Release 1.9.2023112301.
* Thu Nov 23 2023 Severalnines <support@severalnines.com> 1.9.2023112300
- Release 1.9.2023112300.
* Wed Nov 22 2023 Severalnines <support@severalnines.com> 1.9.2023112223
- Release 1.9.2023112223.
* Mon Oct 23 2023 Severalnines <support@severalnines.com> 1.9.2023102321
- Release 1.9.2023102321.
* Wed Oct 18 2023 Severalnines <support@severalnines.com> 1.9.2023101818
- Release 1.9.2023101818.
* Fri Oct  6 2023 Severalnines <support@severalnines.com> 1.9.2023100616
- Release 1.9.2023100616.
* Tue Oct  3 2023 Severalnines <support@severalnines.com> 1.9.2023100314
- Release 1.9.2023100314.
* Wed Aug 30 2023 Severalnines <support@severalnines.com> 1.9.2023083001
- Release 1.9.2023083001.
* Fri Aug 25 2023 Severalnines <support@severalnines.com> 1.9.2023082520
- Release 1.9.2023082520.
* Fri Aug 11 2023 Severalnines <support@severalnines.com> 1.9.2023081120
- Release 1.9.2023081120.
* Fri Aug 11 2023 Severalnines <support@severalnines.com> 1.9.2023081119
- Release 1.9.2023081119.
* Fri Aug 11 2023 Severalnines <support@severalnines.com> 1.9.2023081118
- Release 1.9.2023081118.
* Thu May 25 2023 Severalnines <support@severalnines.com> 1.9.2023052518
- Release 1.9.2023052518.
* Thu May 25 2023 Severalnines <support@severalnines.com> 1.9.2023052514
- Release 1.9.2023052514.
* Fri May 19 2023 Severalnines <support@severalnines.com> 1.9.2023051916
- Release 1.9.2023051916.
* Mon May  8 2023 Severalnines <support@severalnines.com> 1.9.2023050818
- Release 1.9.2023050818.
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
* Thu Nov  3 2022 Severalnines <support@severalnines.com> 1.9.2022110415
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
