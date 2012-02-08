
prefix=		/usr/local
includedir=	$(prefix)/include
libdir=		$(prefix)/lib

CC=		gcc
CXX=		g++
CDEBUGFLAGS=	-g
CXXDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CXXOPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE
CXXDEFINES=	-D_GNU_SOURCE
CWARNFLAGS=	-Wall -Wextra
CXXWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CDEFINES)
CXXFLAGS=	$(CXXDEBUGFLAGS) $(CXXOPTFLAGS) $(CXXWARNFLAGS) $(CXXDEFINES)
INSTALL=	/usr/bin/install -c
RANLIB=		ranlib

all: spiegel

spiegel_SOURCE=	\
		spiegel.C common.c commonp.C
spiegel_PRIVHEADERS= \

spiegel_HEADERS= \

spiegel_OBJS= \
	$(patsubst %.c,%.o,$(filter %.c,$(spiegel_SOURCE))) \
	$(patsubst %.C,%.o,$(filter %.C,$(spiegel_SOURCE)))

spiegel_LDADD= \
	-lbfd

spiegel: $(spiegel_OBJS)
	$(LINK.C) -o spiegel $(spiegel_OBJS) $(spiegel_LDADD)

# install: all
# 	$(INSTALL) -d $(DESTDIR)$(includedir)
# 	for hdr in $(libu4c_HEADERS) ; do \
# 	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/$$hdr ;\
# 	done
# 	$(INSTALL) -d $(DESTDIR)$(libdir)
# 	$(INSTALL) -m 644 libu4c.a $(DESTDIR)$(libdir)/libu4c.a
# 	$(RANLIB) $(DESTDIR)$(libdir)/libu4c.a

clean:
	$(RM) spiegel $(spiegel_OBJS)

# check: all
# 	cd tests ; $(MAKE) $@

