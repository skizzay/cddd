# $1 - name
# $2 - source directories
# $3 - third-party shared system libraries
define DEFINE_EXECUTABLE
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/executable.mk $(strip $2)/software.mk
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(foreach dir, $(2), $(call get_objects_for_directory, $(dir)))
ifneq ($(strip $3),)
$(BIN_DIR)/$(strip $1) : $(1)_3RD_PARTY_LINKER_FLAGS:=$3
endif
all: $(BIN_DIR)/$(strip $1)
endef


# $1 - name
# $2 - list of dependencies
define DEFINE_EXECUTABLE_TO_SHARED_DEPENDENCIES
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$2))
$(BIN_DIR)/$(strip $1) : $(1)_SHARED_LIBRARIES+=$2
endef

# $1 - name
# $2 - list of dependencies
define DEFINE_EXECUTABLE_TO_ARCHIVE_DEPENDENCY
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(addsuffix .a,$(addprefix $(LIB_DIR)/lib,$(strip $2)))
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(strip 1)_ARCHIVES+=$(call use_archive, $($(strip $2)_ARCHIVE)) $($(strip $2)_LINKER_FLAGS)
endef

# $1 - name
# $2 - list of dependencies
define DEFINE_EXECUTABLE_TO_WHOLE_ARCHIVE_DEPENDENCY
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(addsuffix .a,$(addprefix $(LIB_DIR)/lib,$(strip $2)))
.SECONDEXPANSION:
$(BIN_DIR)/$(strip $1) : $(1)_ARCHIVES+=$(call use_whole_archive, $($(strip $2)_ARCHIVE)) $($(strip $2)_LINKER_FLAGS)
endef

define DEFINE_SHARED_LIBRARY_LOCATIONS
$(BIN_DIR)/$(strip $1) : $(1)_SHARED_LIBRARY_LOCATIONS+=$2
endef
