all: shared

CFLAGS :=`pkg-config libusb-1.0 --libs`

shared:
	g++ main.cpp -o sample $(CFLAGS)


.PHONY: all
