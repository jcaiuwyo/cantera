cc_sources = mixGasTransport.cpp

INC                  = -I. -I$(top_builddir)/include/ -g
AM_CPPFLAGS          = $(INC)
AM_CXXFLAGS          = $(AM_CPPFLAGS)

LINK                 = -loneD -lzeroD -lequil -lkinetics -ltransport -lthermo 
LINK                += -lctnumerics -lctmath -ltpx -lctspectra -lctbase -lcvode 
LINK                += -lctlapack -lctblas -lctf2c -lm -lstdc++ -lexecstream -pthread
AM_LDFLAGS           = -L$(top_builddir)/build/lib/ 
LIBS                 =  $(LINK)

bin_PROGRAMS         = mixGasTransport
library_includedir   = $(INC)

#-----------------------
# Cantera DH graph test
#-----------------------

mixGasTransport_SOURCES = $(cc_sources)


TESTS_ENVIRONMENT = 
TESTS             = runtest
