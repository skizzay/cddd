# $1 - name
# $2 - source directory
# $3 - linker flags
define DEFINE_ARCHIVE_IMPL
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).a : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/archive.mk $(strip $2)/software.mk
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).a : $(call get_objects_for_directory, $(2))
all: $(LIB_DIR)/lib$(strip $1).a
$(strip $1)_ARCHIVE=$(LIB_DIR)/lib$(strip $1).a
$(strip $1)_LINKER_FLAGS=$(strip $3)
endef

DEFINE_ARCHIVE=$(eval $(call DEFINE_ARCHIVE_IMPL, $(strip $1), $2, $3))

use_archive=-Wl,--no-whole-archive $(1)
use_whole_archive=-Wl,--whole-archive $(1) -Wl,--no-whole-archive
