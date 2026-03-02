# 1. Project Names
CLIENT_TARGET = iec61850_mms_client
SERVER_TARGET = iec61850_mms_server
CURDIR := $(shell pwd)

# 2. Paths
LIBIEC_HOME = ./lib/libiec61850
cJSON_HOME  = ./lib/cJSON

# Adjust these based on where you place the server files and static_model.h
CLIENT_INCLUDE_DIR ?= -I $(CURDIR)/c_src/client/include
SERVER_INCLUDE_DIR ?= -I $(CURDIR)/c_src/server/include
UTILS_DIR ?= -I $(CURDIR)/c_src/utils/include

# 3. Default Rule (Builds both client and server)
all: libiec $(CLIENT_TARGET) $(SERVER_TARGET)

# 4. Compiler & Flags
CC = gcc
CFLAGS = -g -Wall -Wno-unused-function

# 5. Includes
include $(LIBIEC_HOME)/make/target_system.mk
include $(LIBIEC_HOME)/make/stack_includes.mk
include $(LIBIEC_HOME)/make/common_targets.mk

CFLAGS += -I$(LIBIEC_HOME)/src/include
CFLAGS += -I$(LIBIEC_HOME)/src/iec61850/inc
CFLAGS += -I$(LIBIEC_HOME)/src/mms/inc
CFLAGS += -I$(LIBIEC_HOME)/src/common/inc
CFLAGS += -I$(LIBIEC_HOME)/hal/inc
CFLAGS += -I$(LIBIEC_HOME)/src/logging
CFLAGS += -I$(cJSON_HOME)

# 6. Libraries to Link
LIBS = $(LIBIEC_HOME)/build/libiec61850.a
LIBS += -lpthread

# 7. Source Files (Segregated for Client and Server)
CLIENT_SRCS = c_src/client/src/iec61850_mms_client.c \
              c_src/client/src/iec61850_mms_client_loop.c \
              c_src/utils/src/utils.c \
              $(cJSON_HOME)/cJSON.c

# Notice we added static_model.c to the server sources
SERVER_SRCS = c_src/server/src/iec61850_mms_server.c \
              c_src/server/src/iec61850_mms_server_loop.c \
              c_src/server/src/static_model.c \
              c_src/utils/src/utils.c \
              $(cJSON_HOME)/cJSON.c

# 8. Build Rules

# Ensure the core libiec61850 library is built first
libiec:
	cd $(LIBIEC_HOME) && make

# Compile the Client
$(CLIENT_TARGET): $(CLIENT_SRCS)
	@echo "--- Compiling $(CLIENT_TARGET) ---"
	$(CC) $(CFLAGS) $(CLIENT_INCLUDE_DIR) $(UTILS_DIR) -o $(CLIENT_TARGET) $(CLIENT_SRCS) $(LIBS)

# Compile the Server
$(SERVER_TARGET): $(SERVER_SRCS)
	@echo "--- Compiling $(SERVER_TARGET) ---"
	$(CC) $(CFLAGS) $(SERVER_INCLUDE_DIR) $(UTILS_DIR) -o $(SERVER_TARGET) $(SERVER_SRCS) $(LIBS)

# Optional tool to regenerate static_model.c from the .cid file if you change it later
model:
	@echo "--- Generating static_model.c from CID file ---"
	java -jar $(LIBIEC_HOME)/tools/model_generator/genmodel.jar c_src/server/src/simpleIO_direct_control.cid
	mv static_model.c c_src/server/src/
	mv static_model.h c_src/server/include/

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
	cd $(LIBIEC_HOME) && make clean


# 1. Project Name
# TARGET = iec61850_mms_client
# CURDIR := $(shell pwd)

# # 2. Paths
# LIBIEC_HOME = ./lib/libiec61850
# cJSON_HOME  = ./lib/cJSON

# CLIENT_INCLUDE_DIR ?= -I $(CURDIR)/c_src/client/include
# UTILS_DIR ?= -I $(CURDIR)/c_src/utils/include

# # 3. Default Rule (MUST BE FIRST)
# # This tells make: "When I type 'make', do this first."
# all: libiec $(TARGET)

# # 4. Compiler & Flags
# CC = gcc
# CFLAGS = -g -Wall -Wno-unused-function

# # # Add these specific paths for internal library dependencies

# # 5. Includes (Now safe to include after 'all')
# include $(LIBIEC_HOME)/make/target_system.mk
# include $(LIBIEC_HOME)/make/stack_includes.mk
# include $(LIBIEC_HOME)/make/common_targets.mk

# CFLAGS += -I$(LIBIEC_HOME)/src/include
# CFLAGS += -I$(LIBIEC_HOME)/src/iec61850/inc
# CFLAGS += -I$(LIBIEC_HOME)/src/mms/inc
# CFLAGS += -I$(LIBIEC_HOME)/src/common/inc
# CFLAGS += -I$(LIBIEC_HOME)/hal/inc
# CFLAGS += -I$(LIBIEC_HOME)/src/logging
# CFLAGS += -I$(cJSON_HOME)

# # 6. Libraries to Link
# LIBS = $(LIBIEC_HOME)/build/libiec61850.a
# LIBS += -lpthread

# # 7. Source Files
# SRCS = c_src/client/src/iec61850_mms_client.c c_src/client/src/iec61850_mms_client_loop.c c_src/utils/src/utils.c $(cJSON_HOME)/cJSON.c

# # 8. Build Rules

# # Ensure the library is built first
# libiec:
# 	cd $(LIBIEC_HOME) && make

# # Compile the wrapper
# $(TARGET): $(SRCS)
# 	@echo "--- Compiling $(TARGET) ---"
# 	$(CC) $(CFLAGS) $(CLIENT_INCLUDE_DIR) $(UTILS_DIR) -o $(TARGET) $(SRCS) $(LIBS)

# clean:
# 	rm -f $(TARGET)
# 	cd $(LIBIEC_HOME) && make clean