SRCDIR ?= /opt/fpp/src
include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

all: libfpp-LoRa-improved.$(SHLIB_EXT)

OBJECTS_fpp_LoRa_so += src/FPPLoRa.o
LIBS_fpp_LoRa_so += -L$(SRCDIR) -lfpp -ljsoncpp -lhttpserver
CXXFLAGS_src/FPPLoRa.o += -I$(SRCDIR)

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

libfpp-LoRa-improved.$(SHLIB_EXT): $(OBJECTS_fpp_LoRa_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_LoRa_so) $(LIBS_fpp_LoRa_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-LoRa-improved.so $(OBJECTS_fpp_LoRa_so)
