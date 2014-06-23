get_software_dir=$(abspath $(dir $(abspath $(lastword $(filter %/software.mk,$(MAKEFILE_LIST))))))

CONFIG_DIR:=$(realpath $(dir $(realpath $(lastword $(filter Makefile,$(MAKEFILE_LIST))))))
TOP_DIR:=$(abspath $(CONFIG_DIR)/..)
THIS_DIR:=$(abspath $(dir $(abspath $(lastword $(filter Makefile,$(MAKEFILE_LIST))))))
BIN_DIR:=$(abspath $(CONFIG_DIR)/../bin)
LIB_DIR:=$(abspath $(CONFIG_DIR)/../lib)


# $1 - name of third-party library
# $2 - paths of include headers
# $3 - linker flags
define DEFINE_THIRD_PARTY_LIBRARY
$(strip $1)_INCLUDE_DIRS:=$2
$(strip $1)_LINKER_FLAGS:=$3
endef


.SUFFIXES:

.PHONY: all
all:

include $(CONFIG_DIR)/objects.mk
include $(CONFIG_DIR)/executable.mk
include $(CONFIG_DIR)/shared.mk
include $(CONFIG_DIR)/archive.mk

include $(THIS_DIR)/software.mk

$(BIN_DIR)/% :
	@test -d $(@D) || mkdir -p $(@D)
	g++-4.9 -std=c++1y -g3 -ggdb $(filter %.o,$^) -Wl,-R,'$$ORIGIN/../lib' -L$(LIB_DIR) $(addprefix -L,$($*_SHARED_LIBRARY_LOCATIONS)) $(addprefix -l,$($*_SHARED_LIBRARIES)) $(filter .a, $+) -pthread -o $@

$(LIB_DIR)/lib%.so :
	@test -d $(@D) || mkdir -p $(@D)
	g++-4.9 -shared -std=c++1y -g3 -ggdb -Wl,--no-allow-shlib-undefined,--no-undefined $(filter %.o,$^) -L$(LIB_DIR) $(addprefix -L,$($*_SHARED_LIBRARY_LOCATIONS)) $(addprefix -l,$($*_SHARED_LIBRARIES)) $(filter .a, $+) -o $@

$(LIB_DIR)/lib%.a :
	@test -d $(@D) || mkdir -p $(@D)
	ar rs $(filter %.o,$^)

$(BIN_DIR) $(LIB_DIR) :
	@test -d $@ || mkdir -p $@

.PHONY: clean
clean:
ifneq ($(filter $(THIS_DIR)/obj,$(CLEAN_OBJECT_DIRS)),)
	rm -rf $(filter $(THIS_DIR)/obj,$(CLEAN_OBJECT_DIRS))
else
	@echo "No directories to be removed."
endif
	rm -f $(THIS_DIR)/.*.swp $(THIS_DIR)/*~
