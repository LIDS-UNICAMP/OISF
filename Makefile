#================================================
# COMPILING SETTINGS
#================================================
CC = gcc
CFLAGS= -std=gnu11

# Directories -----------------------------------
HOME = .
INC_DIR = $(HOME)/include
SRC_DIR = $(HOME)/src
OBJ_DIR = $(HOME)/obj
BIN_DIR = $(HOME)/bin
DEMO_DIR = $(HOME)/demo
EXT_DIR = $(HOME)/externals

LJPEG_DIR = $(EXT_DIR)/libjpeg
LPNG_DIR = $(EXT_DIR)/libpng
ZLIB_DIR = $(EXT_DIR)/zlib

# Shared files ----------------------------------
INCS = -I$(INC_DIR) -I$(LJPEG_DIR)/include -I$(LPNG_DIR)/include -I$(ZLIB_DIR)/include
LIBS_LD = -L$(LJPEG_DIR)/lib -L$(LPNG_DIR)/lib -L$(ZLIB_DIR)/lib 
LIBS_LINK = -ljpeg -lpng -lm -lz

# Compiler --------------------------------------
IFT_DEBUG = NO
IFT_PARALLEL = NO
ifeq ($(IFT_DEBUG),YES)
	CFLAGS += -Og -g -pedantic -ggdb -pg -Wfatal-errors -Wall -Wextra -DIFT_DEBUG
else
	CFLAGS += -O3

	ifeq ($(IFT_PARALLEL), YES)
		CFLAGS += -fopenmp -pthread -DIFT_PARALLEL
		LIBS_LINK += -lgomp
	endif
endif

#================================================
# RULES
#================================================  
.PHONY: clean externals obj all

all: externals obj

# Compiling -------------------------------------
obj: $(OBJ_DIR)/ift.o \
	 $(OBJ_DIR)/iftOISF.o \
	 $(OBJ_DIR)/iftOSMOX.o \
	 $(OBJ_DIR)/iftOGRID.o 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | externals
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ $(LIBS_LD) $(LIBS_LINK)

$(DEMO_DIR)/%: | obj
	mkdir -p $(BIN_DIR)
	$(eval OBJ_FILES := $(wildcard $(OBJ_DIR)/*.o))
	$(CC) $(CFLAGS) $(INCS) $(OBJ_FILES) $@.c -o $(BIN_DIR)/$(@F) $(LIBS_LD) $(LIBS_LINK)

# External --------------------------------------
externals:
	cd $(LJPEG_DIR); $(MAKE) -j3 ; cd - ;
	cd $(LPNG_DIR); $(MAKE) -j3 ; cd - ;
	cd $(ZLIB_DIR); $(MAKE) -j3 ; cd - ;

# Cleaning --------------------------------------
clean:
	$(RM) $(OBJ_DIR)/* ;
	$(RM) $(BIN_DIR)/* ;

remove: clean
	cd $(LJPEG_DIR); $(MAKE) clean; cd - ;
	cd $(LPNG_DIR); $(MAKE) clean; cd - ;
	cd $(ZLIB_DIR); $(MAKE) clean; cd - ;	
