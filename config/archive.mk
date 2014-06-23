# $1 - name
# $2 - source directory
define DEFINE_ARCHIVE_IMPL
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).a : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/archive.mk $(strip $2)/software.mk
.SECONDEXPANSION:
$(LIB_DIR)/lib$(strip $1).a : $(call get_objects_for_directory, $(2))
all: $(LIB_DIR)/lib$(strip $1).a
endef

DEFINE_ARCHIVE=$(eval $(call DEFINE_ARCHIVE_IMPL, $(strip $1), $2))

use_archive=-Wl,--no-whole-archive $(1)
use_whole_archive=-Wl,--whole-archive $(1) -Wl,--no-whole-archive
