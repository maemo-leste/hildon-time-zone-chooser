lib_LTLIBRARIES = libhildon_time_zone_chooser0.la

libhildon_time_zone_chooser0_la_CFLAGS = \
		$(HILDON_CFLAGS) $(CITYINFO_CFLAGS) $(TIME_CFLAGS) \
		$(X11_CFLAGS) $(GDK_CFLAGS) -I$(srcdir)/../include

libhildon_time_zone_chooser0_la_LDFLAGS = \
		-Wl,--as-needed $(HILDON_LIBS) $(CITYINFO_LIBS) $(TIME_LIBS) \
		$(X11_LIBS) $(GDK_LIBS) -Wl,--no-undefined

libhildon_time_zone_chooser0_la_SOURCES = \
		hildon-time-zone-chooser.c \
		hildon-time-zone-search.c \
		hildon-time-zone-pannable-map.c

MAINTAINERCLEANFILES = Makefile.in
