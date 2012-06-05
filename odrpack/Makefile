# This makefile downloads and compiles source code for odrpack (the
# double precision version)

DODR_SOURCE = d_lpkbls.f d_odr.f d_mprec.f
DODR_OBJECTS = $(patsubst %.f, %.o, $(DODR_SOURCE))
DODR_CONVERTED = $(patsubst %.f, %.c, $(DODR_SOURCE))

c: $(DODR_CONVERTED)

%.c: %.f
	f2c $<

lib: dodr.a($(DODR_OBJECTS))

$(DODR_SOURCE):
	wget http://www.netlib.org/odrpack/$@

