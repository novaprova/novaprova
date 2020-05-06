Summary: The new generation unit test framework for C
Name: novaprova
Version: @PACKAGE_VERSION@
Release: 1
License: Apache-2.0
Group: Development/Libraries/C and C++
Source: http://sourceforge.net/projects/novaprova/files/novaprova-%{version}.tar.gz
@ifdef release
Source1: novaprova-manual-%{version}.tar.bz2
@endif
Url: http://www.novaprova.org/
BuildRoot: /var/tmp/%{name}-root
BuildRequires: autoconf, automake, gcc-c++ >= 4.8
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
automake -ac || echo woopsie
autoreconf -iv
%configure

make all
make V=1 check

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
%package devel
Summary: The new generation unit test framework for C
Group: Development/Libraries/C and C++
Requires: valgrind, binutils-devel
Obsoletes: novaprova
Provides: novaprova

%description devel
Novaprova is the newest way to organise and run unit tests for libraries
and programs written in the C language. Novaprova takes the xUnit
paradigm into the 21st century.

Novaprova has many advanced features previously only available in unit
test frameworks for languages such as Java or Perl.

This package includes the header files and libraries you need to use Novaprova.

%files devel
%defattr(-,root,root)
%{_includedir}/novaprova/
%{_libdir}/libnovaprova.a
%{_libdir}/pkgconfig/novaprova.pc
%doc README.md LICENSE ChangeLog

%package doc
Summary: Documentation for the new generation unit test framework for C
Group: Development/Libraries/C and C++

%description doc
Novaprova is the newest way to organise and run unit tests for libraries
and programs written in the C language. Novaprova takes the xUnit
paradigm into the 21st century.

Novaprova has many advanced features previously only available in unit
test frameworks for languages such as Java or Perl.

This package includes documentation for using and hacking on Novaprova.

%files doc
%defattr(-,root,root)
@ifdef release
%doc ../novaprova-manual-%{version}/manual
@endif
%docdir %{_mandir}
%{_mandir}/man3/np*.3*
%{_mandir}/man3/NP*.3*

%changelog
