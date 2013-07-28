#settings 
LIBRARIES	= -lm -lhdf5 -lhdf5_hl

# EUMETSAT Wavelet library
EUM_WAVELET_DIR  = /home/deneke/src/eum_wavelet/src/2.04
EUM_WAVELET_LIB  = $(EUM_WAVELET_DIR)/libeum_wavelet.a
EUM_WAVELET_INC  = $(EUM_WAVELET_DIR)/CAPI/

# C Compiler settings
CC		= gcc
CFLAGS		= -I$(EUM_WAVELET_INC) -Wall -std=gnu99 -O3 -g

# Linker
LD	        = g++
LDFLAGS		= $(LIBRARIES)

# Executables
EXES  = msevi_l15_hrit2hdf msevi_l15_hrit2pgm 
#msevi_angles msevi_pro_info
COBJ  =	msevi_l15.o msevi_l15hrit.o cgms_xrit.o msevi_l15hdf.o geos.o \
	sunpos.o timeutils.o memutils.o h5utils.o fileutils.o cds_time.o  

all: $(EXES)

msevi_l15_hrit2hdf: msevi_l15_hrit2hdf.o $(COBJ) $(EUM_WAVELET_LIB)
	$(LD) $(LDFLAGS) -o $@ $^
msevi_l15_hrit2pgm: msevi_l15_hrit2pgm.o $(COBJ) $(EUM_WAVELET_LIB)
	$(LD) $(LDFLAGS) -o $@ $^
msevi_angles: msevi_angles.o $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^
msevi_pro_info: msevi_pro_info.o $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^

$(COBJ): %.o: %.c
	$(CC) -I$(EUM_WAVELET_INC) -c $(CFLAGS) $< -o $@

.PHONY:	clean distclean
 
clean:
	@echo "Cleaning up"
	@\rm -f *.o
	@\rm -f *~
	@\rm -f \#*
	@\rm -f core

distclean: clean
	rm -f $(EXES)	
