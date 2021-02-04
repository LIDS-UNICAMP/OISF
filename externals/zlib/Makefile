# makefile for zlib using gcc (generic, static library)
#
# This code is released under the zlib license.
# For conditions of distribution and use, see the disclaimer
# and license in zlib.h
#CC= cc
LIB=./lib
INCLUDESDIR = ./include
SRC=./src
OBJ=./obj
BIN=./bin

INCLUDE = -I$(INCLUDESDIR)
#LOC = -DASMV
#LOC = -DZLIB_DEBUG -g
CFLAGS = $(LOC) -fPIC -O3 -std=gnu11


zlib: $(LIB)/libz.a
	echo "libz.a built..."

$(LIB)/libz.a: \
$(OBJ)/adler32.o \
$(OBJ)/crc32.o \
$(OBJ)/deflate.o \
$(OBJ)/gzclose.o \
$(OBJ)/gzlib.o \
$(OBJ)/gzread.o \
$(OBJ)/gzwrite.o \
$(OBJ)/infback.o \
$(OBJ)/inffast.o \
$(OBJ)/inflate.o \
$(OBJ)/inftrees.o \
$(OBJ)/trees.o \
$(OBJ)/uncompr.o \
$(OBJ)/zutil.o \

	ar csr $(LIB)/libz.a \
$(OBJ)/*.o

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< $(INCLUDE) -o $@

clean:
	rm -f $(LIB)/libz*.a; rm -f $(OBJ)/*.o;
