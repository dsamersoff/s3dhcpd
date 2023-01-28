PROJ=s3dhcpd
VERSION="2.502"

PREFIX?=/opt
FLAVOR=debug

OS:=$(shell uname)
OS_VER:=$(shell uname -r)
ARCH:=$(shell uname -m)
TIP:=$(shell hg tip -q)

$(shell if [ ! -d obj ] ; then mkdir obj; fi)

TARGETS=$(PROJ)
SRC=.

include $(SRC)/s3dhcpd.mk

install:
	install -m 0500 $(PROJ) $(PREFIX)/sbin/
	install -m 0664 etc/$(PROJ)-example.rc $(PREFIX)/etc/
	install -d $(PREFIX)/etc/rc.d
	install -m 0544 etc/$(PROJ).$(OS)  $(PREFIX)/etc/rc.d/$(PROJ)
