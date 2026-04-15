CURDIR := $(shell pwd)
BASEDIR := $(abspath $(CURDIR)/..)

PROJECT ?= $(notdir $(BASEDIR))
PROJECT := $(strip $(PROJECT))

BUILD ?= $(BASEDIR)/_build
ifeq "$(wildcard $(BUILD) )" ""
	BUILD := $(abspath $(BASEDIR)/../../..)
endif

C_SRC_OUTPUT_CLIENT ?= $(BASEDIR)/priv/iec61850_mms_client
C_SRC_OUTPUT_SERVER ?= $(BASEDIR)/priv/iec61850_mms_server

# eport_c and cJSON libraries
EPORT_C_INCLUDE_DIR ?= $(BUILD)/default/lib/eport_c/priv/include
EPORT_C_LIB_DIR ?= $(BUILD)/default/lib/eport_c/priv

CJSON_INCLUDE_DIR ?= $(BASEDIR)/lib/cJSON

# libiec61850 paths
LIBIEC_HOME ?= $(BASEDIR)/lib/libiec61850
LIBIEC61850_INCLUDE_DIR := $(LIBIEC_HOME)/src/iec61850/inc
LIBIEC61850_LIB_DIR := $(LIBIEC_HOME)/build

UTILITIES_INCLUDE_DIR ?= $(CURDIR)/utils/include
CLIENT_INCLUDE_DIR ?= $(CURDIR)/client/include
SERVER_INCLUDE_DIR ?= $(CURDIR)/server/include
TESTS_INCLUDE_DIR ?= $(CURDIR)/tests

INCLUDE_DIR := -I $(EPORT_C_INCLUDE_DIR) -I $(CJSON_INCLUDE_DIR) \
               -I $(UTILITIES_INCLUDE_DIR) -I $(LIBIEC61850_INCLUDE_DIR) \
               -I $(LIBIEC61850_INCLUDE_DIR)/libiec61850

C_SRC_DIR := $(CURDIR)

include $(LIBIEC_HOME)/make/target_system.mk
include $(LIBIEC_HOME)/make/stack_includes.mk
include $(LIBIEC_HOME)/make/common_targets.mk

INCLUDE_DIR += $(INCLUDES)

UNAME_SYS := $(shell uname -s)
HOST_ARCH := $(shell uname -m)
ifeq ($(UNAME_SYS), Darwin)
	CC ?= cc
	CFLAGS ?= -O3 -arch $(HOST_ARCH) -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -arch $(HOST_ARCH) -finline-functions -Wall
	LDFLAGS ?= -arch $(HOST_ARCH) -flat_namespace -undefined suppress
else ifeq ($(UNAME_SYS), FreeBSD)
	CC ?= cc
	CFLAGS ?= -O3 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
else ifeq ($(UNAME_SYS), Linux)
	CC ?= gcc
	CFLAGS ?= -O3 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
	LDFLAGS += -Wl,-rpath,$(LIBIEC61850_LIB_DIR):$(LIBIEC61850_LIB64_DIR)
endif

CFLAGS += -fPIC $(INCLUDE_DIR)
CXXFLAGS += -fPIC $(INCLUDE_DIR)

CLIENT_CFLAGS := $(CFLAGS) -I $(CLIENT_INCLUDE_DIR)
CLIENT_CXXFLAGS := $(CXXFLAGS) -I $(CLIENT_INCLUDE_DIR)

SERVER_CFLAGS := $(CFLAGS) -I $(SERVER_INCLUDE_DIR)
SERVER_CXXFLAGS := $(CXXFLAGS) -I $(SERVER_INCLUDE_DIR)

LDLIBS += -lpthread -L $(EPORT_C_LIB_DIR) -leport_c -lcjson -lcjson_utils \
          -L $(LIBIEC61850_LIB_DIR) -liec61850

c_verbose_0 = @echo " C     " $(?F);
c_verbose = $(c_verbose_$(V))

cpp_verbose_0 = @echo " CPP   " $(?F);
cpp_verbose = $(cpp_verbose_$(V))

link_verbose_0 = @echo " LD    " $(@F);
link_verbose = $(link_verbose_$(V))

CLIENT_SOURCES := $(shell find $(C_SRC_DIR) -type d \( -name "libiec61850" -o -name "server" -o -name "tests" \) -prune -false -o -type f \( -name "*.c" -o -name "*.C" -o -name "*.cc" -o -name "*.cpp" \))
CLIENT_OBJECTS = $(addsuffix .o, $(basename $(CLIENT_SOURCES)))

SERVER_SOURCES := $(shell find $(C_SRC_DIR) -type d \( -name "libiec61850" -o -name "client" -o -name "tests" \) -prune -false -o -type f \( -name "*.c" -o -name "*.C" -o -name "*.cc" -o -name "*.cpp" \))
SERVER_OBJECTS = $(addsuffix .o, $(basename $(SERVER_SOURCES)))

CONNECT_SMOKE_OUTPUT ?= $(BASEDIR)/iec61850_mms_client_connect_smoke
CONNECT_SMOKE_SOURCES := \
	$(CURDIR)/tests/iec61850_mms_client_connect_smoke.c \
	$(CURDIR)/client/src/iec61850_mms_client_loop.c \
	$(CURDIR)/utils/src/utils.c \
	$(BASEDIR)/lib/cJSON/cJSON.c \
	$(BASEDIR)/lib/cJSON/cJSON_Utils.c
CONNECT_SMOKE_OBJECTS = $(addsuffix .o, $(basename $(CONNECT_SMOKE_SOURCES)))
CONNECT_SMOKE_CFLAGS := $(CFLAGS) -I $(CLIENT_INCLUDE_DIR) -I $(TESTS_INCLUDE_DIR)
CONNECT_SMOKE_LDLIBS := -lpthread -L $(LIBIEC61850_LIB_DIR) -liec61850
CONNECT_SMOKE_LDFLAGS :=

ifeq ($(UNAME_SYS), Darwin)
CONNECT_SMOKE_LDFLAGS += -arch $(HOST_ARCH)
endif

$(C_SRC_OUTPUT_CLIENT) : COMPILE_C = $(c_verbose) $(CC) $(CLIENT_CFLAGS) $(CPPFLAGS) -c
$(C_SRC_OUTPUT_CLIENT) : COMPILE_CPP = $(cpp_verbose) $(CXX) $(CLIENT_CXXFLAGS) $(CPPFLAGS) -c

$(C_SRC_OUTPUT_SERVER) : COMPILE_C = $(c_verbose) $(CC) $(SERVER_CFLAGS) $(CPPFLAGS) -c
$(C_SRC_OUTPUT_SERVER) : COMPILE_CPP = $(cpp_verbose) $(CXX) $(SERVER_CXXFLAGS) $(CPPFLAGS) -c

$(CONNECT_SMOKE_OUTPUT) : COMPILE_C = $(c_verbose) $(CC) $(CONNECT_SMOKE_CFLAGS) $(CPPFLAGS) -c
$(CONNECT_SMOKE_OUTPUT) : COMPILE_CPP = $(cpp_verbose) $(CXX) $(CONNECT_SMOKE_CFLAGS) $(CPPFLAGS) -c

all: $(C_SRC_OUTPUT_CLIENT) $(C_SRC_OUTPUT_SERVER)
connect-smoke: $(CONNECT_SMOKE_OUTPUT)

$(C_SRC_OUTPUT_CLIENT): $(CLIENT_OBJECTS)
	@mkdir -p $(BASEDIR)/priv/
	$(link_verbose) $(CC) $(CLIENT_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(C_SRC_OUTPUT_CLIENT)

$(C_SRC_OUTPUT_SERVER): $(SERVER_OBJECTS)
	@mkdir -p $(BASEDIR)/priv/
	$(link_verbose) $(CC) $(SERVER_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(C_SRC_OUTPUT_SERVER)

$(CONNECT_SMOKE_OUTPUT): $(CONNECT_SMOKE_OBJECTS)
	$(link_verbose) $(CC) $(CONNECT_SMOKE_OBJECTS) $(CONNECT_SMOKE_LDFLAGS) $(CONNECT_SMOKE_LDLIBS) -o $(CONNECT_SMOKE_OUTPUT)

%.o: %.c
	$(COMPILE_C) $(OUTPUT_OPTION) $<

%.o: %.cc
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.C
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.cpp
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

clean:
	@rm -f $(C_SRC_OUTPUT_CLIENT) $(C_SRC_OUTPUT_SERVER) $(CONNECT_SMOKE_OUTPUT) \
		$(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(CONNECT_SMOKE_OBJECTS)
