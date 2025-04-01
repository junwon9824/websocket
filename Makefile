
#
#   iNance Xt Server Platform V1.0
#   Copyright (c) iNance Co.,Ltd.
#   All right reserved.
#   2017.08.22
#
BINDIR = ./

SYSLIBS = -lm $(SOCKET_LIB) $(THREAD_LIB)
USRLIBS = -L$(XTHOME)/lib -lXtLib -lxtp -L. -lbase64 -lssl  -lcrypto


INCLUDE     = -I$(XTHOME)/include

CFLAGS      = -D$(_OSTYPE_) $(CFLAG) $(INCLUDE)
CPPFLAGS    = -D$(_OSTYPE_) $(CFLAG) $(INCLUDE)
COMPILE.c   = $(CC) -c
COMPILE.cpp = $(CPP) -c

.SUFFIXES : .c .o .cc
.c.o   :
	$(COMPILE.c) $< $(CFLAGS)
.cc.o   :
	$(COMPILE.cpp) $< $(CPPFLAGS)

all : client server libbase64.a

# 정적 라이브러리 포함
libbase64.a: base64.o
	ar rcs libbase64.a base64.o


client : client.o libbase64.a 
	$(CC) -o $@ $(CFLAGS) $@.o $(SYSLIBS) $(USRLIBS)
	strip $(STRIPFLAG) $@

#server : server.o libbase64.a
server : server.o
	$(CC) -o $@ $(CFLAGS) $@.o $(SYSLIBS) $(USRLIBS) -L ./libbase64
	strip $(STRIPFLAG) $@

clean:
	rm -f client.o server.o libbase64.a base64.o

