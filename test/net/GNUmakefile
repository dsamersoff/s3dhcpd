PROJ=test
VERSION="1.001"

FLAVOR=debug

$(shell if [ ! -d obj ] ; then mkdir obj; fi)

TARGETS=$(PROJ)
SRC=../..

EXTRA_CXXFLAGS=-DUNIT_TEST

include $(SRC)/s3dhcpd.mk
