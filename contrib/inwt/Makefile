prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
NEWT		= $(bindir)/newt
SCRIPT		= inwt

all:		$(SCRIPT)

edit = sed \
	-e 's,@NEWT\@,$(NEWT),g'

$(SCRIPT):	$(SCRIPT).nwt
	$(edit) $(SCRIPT).nwt > $@

install::
	install -m 755 $(SCRIPT) $(DESTDIR)$(bindir)

clean:
	rm -f $(SCRIPT)
