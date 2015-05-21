all: instbench

NDK_PREBUILT=$(HOME)/a/android-ndk-r10d/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin
ANDROID_PLATFORM=$(HOME)/a/android-ndk-r10d/platforms/android-21/arch-arm/usr

TARGET=ANDROID

ifeq ($(TARGET),ANDROID)
CC=$(NDK_PREBUILT)/arm-linux-androideabi-gcc
CXX=$(NDK_PREBUILT)/arm-linux-androideabi-g++
SYSROOT=--sysroot ${ANDROID_PLATFORM}
CFLAGS_SYSDEP=-I${ANDROID_PLATFORM}/include -L${ANDROID_PLATFORM}/lib $(SYSROOT)
else
CC=gcc
CXX=g++
CFLAGS_SYSDEP=-DEMIT_ONLY
endif

WARN_FLAGS=-Wall -Werror=missing-prototypes -Werror=implicit-function-declaration # -Werror=unknown-pragmas

CFLAGS_COMMON=$(WARN_FLAGS) -O0 -ffast-math -falign-loops -MD -fvisibility=hidden -I$(CURDIR) -g
CFLAGS_COMMON+=$(CFLAGS_SYSDEP)

CFLAGS=$(CFLAGS_COMMON) -std=gnu99
CXXFLAGS=$(CFLAGS_COMMON) -std=gnu++11

C_SRCS=ag/ag_gen.c npr/varray.c npr/mempool-c.c
CXX_SRCS=main.cpp # gentest.cpp

OBJS_REL=$(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
OBJS = $(foreach obj,$(OBJS_REL),$(CURDIR)/$(obj))

instbench: $(OBJS)
	$(CXX) $(SYSROOT) $(LDFLAGS) -o $@ $^

libag.a: ag/ag_gen.o npr/varray.o npr/mempool-c.o
	ar cru $@ $^

gentest: gentest.cpp ag/ag_gen.c npr/varray.c npr/mempool-c.c
	gcc -g -std=gnu99 -I$(CURDIR) -o $@ $^

DEPS=$(OBJS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) gentest