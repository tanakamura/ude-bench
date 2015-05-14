all: gentest

#CC=arm-linux-androideabi-gcc
#CXX=arm-linux-androideabi-g++
CC=gcc
CXX=g++

WARN_FLAGS=-Wall -Werror=missing-prototypes -Werror=implicit-function-declaration # -Werror=unknown-pragmas
CFLAGS_COMMON= $(WARN_FLAGS) -O0 -ffast-math -falign-loops -MD -fvisibility=hidden -I$(CURDIR) -g
CFLAGS=$(CFLAGS_COMMON) -std=gnu99
CXXFLAGS=$(CFLAGS_COMMON)

C_SRCS=ag/ag_gen.c npr/varray.c npr/mempool-c.c
CXX_SRCS=main.cpp

OBJS_REL=$(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
OBJS = $(foreach obj,$(OBJS_REL),$(CURDIR)/$(obj))

gentest: $(OBJS)
	g++ $(LDFLAGS) -o $@ $^

DEPS=$(OBJS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) gentest