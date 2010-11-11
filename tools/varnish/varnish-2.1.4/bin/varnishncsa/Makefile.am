# $Id: Makefile.am 5285 2010-09-27 12:40:31Z tfheen $

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = varnishncsa

if HAVE_RST2MAN
dist_man_MANS = varnishncsa.1
endif

varnishncsa_SOURCES = varnishncsa.c

varnishncsa_LDADD = \
	$(top_builddir)/lib/libvarnish/libvarnish.la \
	$(top_builddir)/lib/libvarnishcompat/libvarnishcompat.la \
	$(top_builddir)/lib/libvarnishapi/libvarnishapi.la \
	${PTHREAD_LIBS}

%.1: ../../doc/sphinx/reference/%.rst
if HAVE_RST2MAN
	${RST2MAN} $< >$@
else
	@echo "========================================"
	@echo "You need rst2man installed to make dist"
	@echo "========================================"
	@false
endif
