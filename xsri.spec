Summary:   X Set Root Image
Name:      xsri
Version:   2.1.0
Release:   17%{?dist}
Epoch:     1
License:   GPL+
Group:     System Environment/Base
Source0:   %{name}-%{version}.tar.gz
Source1:   xsri.1

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: gtk2-devel >= 1.3.13
BuildRequires: popt-devel

%description
X Set Root Image - displays images on the root window.

%prep
%setup

%build
%configure
make

%install
export CFLAGS=$RPM_OPT_FLAGS
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
install -m 0644 %{SOURCE1} $RPM_BUILD_ROOT%{_mandir}/man1/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc COPYING ChangeLog README NEWS
%{_bindir}/xsri
%{_mandir}/man1/xsri.1*

%changelog
* Mon Jul 27 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1:2.1.0-17
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Thu Feb 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1:2.1.0-16
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Mon Aug 11 2008 Jason L Tibbitts III <tibbs@math.uh.edu> - 2.1.0-15
- Fix license tag.

* Tue Feb 19 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 1:2.1.0-14
- Autorebuild for GCC 4.3

* Mon Oct 22 2007 Adam Jackson <ajax@redhat.com> 1:2.1.0-13
- BuildRequires: popt-devel

* Tue Aug 21 2007 Adam Jackson <ajax@redhat.com> - 1:2.1.0-12
- Rebuild for build id

* Sat Apr 21 2007 Matthias Clasen <mclasen@redhat.com> 1:2.10.0-11
- Don't install INSTALL

* Fri Jul 21 2006 Mike A. Harris <mharris@redhat.com> 1:2.1.0-10.fc6
- Added manpage for xsri (#13197)
- Use {dist} tag in release field.
- Add missing docs to doc tag.

* Wed Jul 12 2006 Jesse Keating <jkeating@redhat.com> 1:2.1.0-9.2.2
- rebuild

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> 1:2.1.0-9.2.1
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> 1:2.1.0-9.2
- rebuilt for new gcc4.1 snapshot and glibc changes

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Thu Mar  3 2005 Mike A. Harris <mharris@redhat.com> 1:2.1.0-9
- I seem to have just unknowingly won the xsri package in a game of package
  roulette.  I pledge my allegiance to make it the best eye-candy app on the
  desktop for 2005.
- Rebuilt with gcc 4 for FC4
- Make rpm install section export RPM_OPT_FLAGS in CFLAGS

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Wed Dec 11 2002 Tim Powers <timp@redhat.com> 1:2.1.0-4
- rebuild on all arches

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Thu May 23 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Mon Feb 11 2002 Owen Taylor <otaylor@redhat.com>
- Version 2.1.0

* Wed Aug 15 2001 Owen Taylor <otaylor@redhat.com>
- Version 2.0.3

* Fri Aug 03 2001 Owen Taylor <otaylor@redhat.com>
- Version 2.0.2

* Sun Jul 08 2001 Owen Taylor <otaylor@redhat.com>
- Version 2.0.1

* Sun Jul 08 2001 Owen Taylor <otaylor@redhat.com>
- Epoch is 1

* Sun Jul 08 2001 Owen Taylor <otaylor@redhat.com>
- Version 2.0.0

* Sun Jul  8 2001 Owen Taylor <otaylor@redhat.com>
- Completely new version

* Tue Apr 6 1999 The Rasterman <raster@redhat.com>
- made rpm
