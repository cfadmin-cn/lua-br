.PHONY : build clean

default :
	@echo "======================================="
	@echo "Please use 'make build' command to build it.."
	@echo "Please use 'make clean' command to clean all."
	@echo "======================================="

CC = cc
RM = rm -rf
MV = mv

INCLUDES += -I../../src -I../../../src -I/usr/local/include
LIBS += -L../ -L../../ -L../../../ -L/usr/local/lib
CFLAGS += -O3 -shared -fPIC
MICRO += -Wl,-rpath,. -Wl,-rpath,.. -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/local/lib64
DLL += -lcore -lbrotlienc -lbrotlidec

build:
	@$(CC) -o lbr.so lua-br.c $(INCLUDES) $(LIBS) $(CFLAGS) $(MICRO) $(DLL)
	@$(MV) *.so ../

clean:
	@$(RM) main *.so
