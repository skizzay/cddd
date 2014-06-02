# $1 - name
# $2 - source directory
# $3 - shared system libraries
define DEFINE_EXECUTABLE
$(info Defining executable: $(BIN_DIR)/$(strip $1))
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/executable.mk $(strip $2)/software.mk
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(call get_objects_for_directory, $(2))
ifneq ($(strip $3),)
$(BIN_DIR)/$(strip $1) : $(1)_SHARED_LIBRARIES+=$3
endif
all: $(BIN_DIR)/$(strip $1)
endef


# $1 - name
# $2 - list of dependencies
define DEFINE_EXECUTABLE_TO_SHARED_DEPENDENCIES
$(BIN_DIR)/$(strip $1) : $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$2))
$(BIN_DIR)/$(strip $1) : $(1)_SHARED_LIBRARIES+=$2
endef

define DEFINE_SHARED_LIBRARY_LOCATIONS
$(BIN_DIR)/$(strip $1) : $(1)_SHARED_LIBRARY_LOCATIONS+=$2
endef
