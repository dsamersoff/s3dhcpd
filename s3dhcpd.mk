#
# GNU make include 
#


.PHONY: all clean

all: $(TARGETS)

include $(SRC)/src/dhcp/s3_dhcp.mk
include $(SRC)/src/alloc/s3_alloc.mk
include $(SRC)/src/tools/s3_tools.mk
include $(SRC)/src/radius/s3_radius.mk
include $(SRC)/src/share/s3_share.mk
include $(SRC)/src/pf/s3_pf.mk

ifeq ($(OS), Linux)
  OS_DEFS=-DLinux
  PIC=-fPIC
  LIBS+= -lrt -lsqlite3 -lpcap
endif

ifeq ($(OS), FreeBSD)
  OS_DEFS=-DFreeBSD
  PIC=-fPIC
  INCLUDES+= -I/usr/local/include
  LIBS+= -lpcap -L/usr/local/lib -lsqlite3
  ifeq ($(OS_VER), 10.1-RELEASE)
    CXX=clang++
  endif

  OS_DEFS+=-DWITH_PF 
  OBJS+= $(OBJS_PF)
  INCLUDES+= $(INCLUDE_PF)
endif

ifeq ($(OS), Darwin)
  OS_DEFS=-DDarwin -D_XOPEN_SOURCE
  PIC=-fPIC
  LIBS+= -lpcap -lsqlite3
  CXX=clang++

  OS_DEFS+= -DWITH_PF
  OBJS+= $(OBJS_PF)
  # Apple doesn't supply pfvar.h header,
  # so use private copy extracted from xnu sources
  # http://www.opensource.apple.com/tarballs/xnu/
  INCLUDES+= $(INCLUDE_PF)/xnu
  INCLUDES+= $(INCLUDE_PF)
endif

OBJS+=$(OBJS_DHCP) $(OBJS_ALLOC) $(OBJS_TOOLS) $(OBJS_SHARE) obj/main.o
INCLUDES+=$(INCLUDE_DHCP) $(INCLUDE_ALLOC) $(INCLUDE_TOOLS) $(INCLUDE_SHARE)

OS_DEFS+=-D$(ARCH)

OS_DEFS+= -DWITH_RADIUS_ACCT
OBJS+= $(OBJS_RADIUS)
INCLUDES+= $(INCLUDE_RADIUS)

CXXFLAGS+=$(PIC) -I. -I./src -Wno-deprecated-declarations  -Werror $(OS_DEFS) $(ARCH_DEFS) $(EXTRA_CXXFLAGS) $(INCLUDES)
# LDFLAGS=

ifeq ($(FLAVOR),product)
   CXXFLAGS+=-O3
endif 

ifeq ($(FLAVOR),debug)
  CXXFLAGS+=-gdwarf-2
  LDFLAGS+=-gdwarf-2
  OS_DEFS+=-DENABLE_ASSERTS
endif 

clean:
	rm -f $(PROJ).so $(PROJ)
	rm -f obj/*.o
	rm -f obj/VERSION.o obj/VERSION.cxx
	rm -f core.*


$(PROJ): $(OBJS) obj/VERSION.o
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) obj/VERSION.o $(LIBS)

obj/main.o: src/main.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/VERSION.o: obj/VERSION.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

obj/VERSION.cxx: $(OBJS)
	echo "const char *VERSION(){ return \"${PROJ} ${VERSION} `date \"+%d-%m-%Y %H:%S\"` $(TIP) $(OS_DEFS)\"; }" > obj/VERSION.cxx
