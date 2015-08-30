Summary: novaprova, the new generation unit test framework for C
Name: novaprova
Version: 1.3
Release: 1
License: GPL
Group: Development/C
Source: http://sourceforge.net/projects/novaprova/files/novaprova-%{version}.tar.gz
@ifdef release
Source1: novaprova-manual-%{version}.tar.bz2
@endif
Url: http://www.novaprova.org/
BuildRoot: /var/tmp/%{name}-root
Requires: valgrind, binutils-devel
BuildRequires: autoconf, automake, gcc-c++
BuildRequires: valgrind-devel, binutils-devel, libxml2-devel, pkgconfig
BuildRequires: doxygen, perl-XML-LibXML
Vendor: Greg Banks <gnb@fmeh.org>

%description
Novaprova is the newest way to organise and run unit tests for libraries
and programs written in the C language. Novaprova takes the xUnit
paradigm into the 21st century.

Novaprova has many advanced features previously only available in unit
test frameworks for languages such as Java or Perl.

%if %{_vendor} == "suse"
%debug_package
%endif

%prep
@ifdef release
%setup -q -b 1
@else
%setup -q
@endif

%build
autoreconf -iv
%configure

make all
make V=1 check
@ifdef release
make docs
@endif

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_includedir}/novaprova/
%{_libdir}/libnovaprova.a
%{_libdir}/pkgconfig/novaprova.pc
@ifdef release
%doc build/manual
@endif
%docdir %{_mandir}
%{_mandir}/man3/np*.3*
%{_mandir}/man3/NP*.3*
%doc README.md LICENSE ChangeLog

%changelog
