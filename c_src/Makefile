# Based on c_src.mk from erlang.mk by Loic Hoguin <essen@ninenines.eu>

CURDIR := $(shell pwd)
BASEDIR := $(abspath $(CURDIR)/..)

PROJECT ?= $(notdir $(BASEDIR))
PROJECT := $(strip $(PROJECT))

BUILD ?= $(BASEDIR)/_build
ifeq "$(wildcard $(BUILD) )" ""
	BUILD := $(abspath $(BASEDIR)/../../..)
endif

# eport_c and cJSON libraries
EPORT_C_INCLUDE_DIR ?= $(BUILD)/default/lib/eport_c/priv/include
EPORT_C_LIB_DIR ?= $(BUILD)/default/lib/eport_c/priv
CJSON_INCLUDE_DIR ?= $(BUILD)/default/lib/eport_c/priv/cjson

# libiec61850 paths (compiled via build_deps.sh)
LIBIEC_HOME ?= $(CURDIR)/libiec61850
LIBIEC61850_INCLUDE_DIR ?= $(LIBIEC_HOME)/build/_install/include
LIBIEC61850_LIB64_DIR ?= $(LIBIEC_HOME)/build/_install/lib64
LIBIEC61850_LIB_DIR ?= $(LIBIEC_HOME)/build/_install/lib

UTILITIES_INCLUDE_DIR ?= $(CURDIR)/c_src/utils/include
CLIENT_INCLUDE_DIR ?= $(CURDIR)/c_src/client/include
SERVER_INCLUDE_DIR ?= $(CURDIR)/c_src/server/include

# Add standard include dir and the nested libiec61850 dir to catch all headers
INCLUDE_DIR = -I $(EPORT_C_INCLUDE_DIR) -I $(CJSON_INCLUDE_DIR) \
              -I $(UTILITIES_INCLUDE_DIR) -I $(LIBIEC61850_INCLUDE_DIR) \
              -I $(LIBIEC61850_INCLUDE_DIR)/libiec61850

C_SRC_DIR = $(CURDIR)
C_SRC_OUTPUT_CLIENT ?= $(BASEDIR)/priv/$(PROJECT)_client
C_SRC_OUTPUT_SERVER ?= $(BASEDIR)/priv/$(PROJECT)_server

include $(CURDIR)/c_src/libiec61850/make/target_system.mk
include $(CURDIR)/c_src/libiec61850/make/stack_includes.mk
include $(CURDIR)/c_src/libiec61850/make/common_targets.mk

# System type and C compiler/flags.
UNAME_SYS := $(shell uname -s)
ifeq ($(UNAME_SYS), Darwin)
	CC ?= cc
	CFLAGS ?= -O3 -arch x86_64 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -arch x86_64 -finline-functions -Wall
	LDFLAGS ?= -arch x86_64 -flat_namespace -undefined suppress
else ifeq ($(UNAME_SYS), FreeBSD)
	CC ?= cc
	CFLAGS ?= -O3 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
else ifeq ($(UNAME_SYS), Linux)
	CC ?= gcc
	CFLAGS ?= -O3 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
endif

CFLAGS += -fPIC $(INCLUDE_DIR)
CXXFLAGS += -fPIC $(INCLUDE_DIR)

CLIENT_CFLAGS = $(CFLAGS) -I $(CLIENT_INCLUDE_DIR)
CLIENT_CXXFLAGS += $(CXXFLAGS) -I $(CLIENT_INCLUDE_DIR)

SERVER_CFLAGS = $(CFLAGS) -I $(SERVER_INCLUDE_DIR)
SERVER_CXXFLAGS += $(CXXFLAGS) -I $(SERVER_INCLUDE_DIR)

LDLIBS += -lpthread -L $(EPORT_C_LIB_DIR) -leport_c -lcjson -lcjson_utils \
          -L $(LIBIEC61850_LIB_DIR) -L $(LIBIEC61850_LIB64_DIR) -liec61850

# Verbosity.
c_verbose_0 = @echo " C     " $(?F);
c_verbose = $(c_verbose_$(V))

cpp_verbose_0 = @echo " CPP   " $(?F);
cpp_verbose = $(cpp_verbose_$(V))

link_verbose_0 = @echo " LD    " $(@F);
link_verbose = $(link_verbose_$(V))

# Automatically find sources, ignoring the library, tests, and the opposite target's folder
CLIENT_SOURCES := $(shell find $(C_SRC_DIR) -type d \( -name "libiec61850" -o -name "server" -o -name "tests" \) -prune -false -o -type f \( -name "*.c" -o -name "*.C" -o -name "*.cc" -o -name "*.cpp" \))
CLIENT_OBJECTS = $(addsuffix .o, $(basename $(CLIENT_SOURCES)))

SERVER_SOURCES := $(shell find $(C_SRC_DIR) -type d \( -name "libiec61850" -o -name "client" -o -name "tests" \) -prune -false -o -type f \( -name "*.c" -o -name "*.C" -o -name "*.cc" -o -name "*.cpp" \))
SERVER_OBJECTS = $(addsuffix .o, $(basename $(SERVER_SOURCES)))

$(C_SRC_OUTPUT_CLIENT) : COMPILE_C = $(c_verbose) $(CC) $(CLIENT_CFLAGS) $(CPPFLAGS) -c
$(C_SRC_OUTPUT_CLIENT) : COMPILE_CPP = $(cpp_verbose) $(CXX) $(CLIENT_CXXFLAGS) $(CPPFLAGS) -c

$(C_SRC_OUTPUT_SERVER) : COMPILE_C = $(c_verbose) $(CC) $(SERVER_CFLAGS) $(CPPFLAGS) -c
$(C_SRC_OUTPUT_SERVER) : COMPILE_CPP = $(cpp_verbose) $(CXX) $(SERVER_CXXFLAGS) $(CPPFLAGS) -c

all: $(C_SRC_OUTPUT_CLIENT) $(C_SRC_OUTPUT_SERVER)

$(C_SRC_OUTPUT_CLIENT): $(CLIENT_OBJECTS)
	@mkdir -p $(BASEDIR)/priv/
	$(link_verbose) $(CC) $(CLIENT_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(C_SRC_OUTPUT_CLIENT)

$(C_SRC_OUTPUT_SERVER): $(SERVER_OBJECTS)
	@mkdir -p $(BASEDIR)/priv/
	$(link_verbose) $(CC) $(SERVER_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(C_SRC_OUTPUT_SERVER)

%.o: %.c
	$(COMPILE_C) $(OUTPUT_OPTION) $<

%.o: %.cc
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.C
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.cpp
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

clean:
	@rm -f $(C_SRC_OUTPUT_CLIENT) $(C_SRC_OUTPUT_SERVER) $(CLIENT_OBJECTS) $(SERVER_OBJECTS)