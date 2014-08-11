# $1 - name
# $2 - source directory
# $3 - third-party linker flags
define DEFINE_SHARED_LIBRARY
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).so : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/shared.mk $(strip $2)/software.mk
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).so : $(call get_objects_for_directory, $(2))
ifneq ($(strip $3),)
$(LIB_DIR)/lib$(strip $1).so : $(1)_3RD_PARTY_LINKER_FLAGS:=$3
endif
all: $(LIB_DIR)/lib$(strip $1).so
endef

# $1 - name
# $2 - list of dependencies
define DEFINE_SHARED_TO_SHARED_DEPENDENCIES
$(LIB_DIR)/lib$(strip $1).so : $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$2))
$(LIB_DIR)/lib$(strip $1).so : $(1)_SHARED_LIBRARIES+=$2
endef

define DEFINE_SHARED_LIBRARY_LOCATIONS
$(LIB_DIR)/lib$(strip $1).so : $(1)_SHARED_LIBRARY_LOCATIONS+=$2
endef
