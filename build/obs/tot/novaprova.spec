Summary: novaprova, the new generation unit test framework for C
Name: novaprova
Version: 1.3
Release: 1
License: GPL
Group: Development/C
Source: http://sourceforge.net/projects/novaprova/files/novaprova-%{version}.tar.gz
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
%setup -q

%build
autoreconf -iv
%configure

make all
make V=1 check

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
%doc doc/inst/*
%doc README.md LICENSE ChangeLog

%changelog
