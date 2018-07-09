# compiler and flags
GCC = gcc
G++ = g++
CFLAGS = -g -O3 -fPIC -shared -mavx -msse4 \
                     -I. -I/usr/local/include \
                     -L. -L/usr/local/lib \
                     -lhashpipe -lrt -lm
                      
                     

# linker options
LFLAGS_CUFFT = -lcufft
LFLAGS_PGPLOT = -L/usr/lib64/pgplot -lpgplot -lcpgplot -lX11

# bin directory
BINDIR = ./bin

# filterbank
FILTERBANK_OBJECT   = filterbank.o
FILTERBANK_SOURCES  = filterbank.cpp
FILTERBANK_LIB_INCLUDES = filterbank.h

# hashpipe
HPDEMO_LIB_TARGET   = dibas_frb_hashpipe.so
HPDEMO_LIB_SOURCES  = dibas_frb_net_thread.c \
		      dibas_frb_output_thread.c \
                      dibas_frb_databuf.c
HPDEMO_LIB_OBJECT = dibas_frb_hashpipe.o
HPDEMO_LIB_INCLUDES = dibas_frb_databuf.h

all: $(FILTERBANK_OBJECT) $(HPDEMO_LIB_TARGET)

$(FILTERBANK_OBJECT): $(FILTERBANK_SOURCES) $(FILTERBANK_LIB_INCLUDES)
	$(G++) -c $< $(CFLAGS)

$(HPDEMO_LIB_TARGET): $(HPDEMO_LIB_SOURCES) $(HPDEMO_LIB_INCLUDES) $(FILTERBANK_OBJECT)
	$(G++) $(HPDEMO_LIB_SOURCES) $(FILTERBANK_OBJECT) -o $@ $(CFLAGS)

tags:
	ctags -R .
clean:
	rm -f $(HPDEMO_LIB_TARGET) *.o tags

prefix=/usr/local
LIBDIR=$(prefix)/lib
BINDIR=$(prefix)/bin
install-lib: $(HPDEMO_LIB_TARGET)
	mkdir -p "$(DESTDIR)$(LIBDIR)"
	install -p $^ "$(DESTDIR)$(LIBDIR)"
install: install-lib

.PHONY: all tags clean install install-lib
# vi: set ts=8 noet :
