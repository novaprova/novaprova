Summary: novaprova, the new generation unit test framework for C
Name: novaprova
Version: 1.1
Release: 1
License: GPL
Group: Development/C
Source: http://sourceforge.net/projects/novaprova/files/novaprova-%{version}.tar.bz2
Url: http://www.novaprova.org/
BuildRoot: /var/tmp/%{name}-root
Requires: valgrind, binutils-devel
BuildRequires: valgrind-devel, binutils-devel, libxml++-devel, libxml2-devel, pkgconfig
Vendor: Greg Banks <gnb@fmeh.org>
Patch1: novaprova-build-install-pc-correctly.patch
Patch2: novaprova-fedora-build-libxml.patch
Patch3: novaprova-build-deps.patch
Patch4: novaprova-build-no-doxygen.patch

%description
Novaprova is the newest way to organise and run unit tests for libraries
and programs written in the C language. Novaprova takes the xUnit
paradigm into the 21st century.

Novaprova has many advanced features previously only available in unit
test frameworks for languages such as Java or Perl.

%define overrides prefix=%{_prefix} exec_prefix=%{_exec_prefix} includedir=%{_includedir} libdir=%{_libdir} mandir=%{_mandir} pkgdocdir=%{_docdir}/%{name}

%prep
%setup -q
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1

%build
make all %{overrides}

%install
rm -rf $RPM_BUILD_ROOT
make install %{overrides} DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_includedir}/novaprova/np.h
%{_includedir}/novaprova/np/*.hxx
%{_libdir}/libnovaprova.a
%{_libdir}/pkgconfig/novaprova.pc
%{_docdir}/novaprova/api-ref/*
%docdir %{_mandir}
%{_mandir}/man3/np.h.3*
%doc README.md LICENSE ChangeLog

%changelog
