# $Id: Makefile.am 5433 2010-10-19 14:12:13Z tfheen $

dist_man_MANS = vcl.7

vcl.7: $(top_srcdir)/doc/sphinx/reference/vcl.rst \
	$(top_srcdir)/bin/varnishd/default.vcl
if HAVE_RST2MAN
	${RST2MAN} $< >$@
else
	@echo "========================================"
	@echo "You need rst2man installed to make dist"
	@echo "========================================"
	@false
endif
