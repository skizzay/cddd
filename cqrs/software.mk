ifndef Cqrs_DIR
Cqrs_DIR:=$(call get_software_dir)

include $(CONFIG_DIR)/sequence.mk
include $(CONFIG_DIR)/coroutine.mk

$(call DEFINE_OBJECTS, $(Cqrs_DIR), $(abspath $(Cqrs_DIR)/../..) $(Sequence_INCLUDE_DIRS))
$(eval $(call DEFINE_SHARED_LIBRARY, Cqrs, $(Cqrs_DIR)))

endif
