ifndef CqrsUnitTests_DIR
CqrsUnitTests_DIR:=$(call get_software_dir)

include $(CONFIG_DIR)/coroutine.mk
include $(CONFIG_DIR)/gmock.mk
include $(TOP_DIR)/cqrs/software.mk

$(call DEFINE_OBJECTS, $(CqrsUnitTests_DIR), $(gmock_INCLUDE_DIRS) $(abspath $(Cqrs_DIR)/../..) $(Sequence_INCLUDE_DIRS))
$(eval $(call DEFINE_ARCHIVE, CqrsUnitTests, $(CqrsUnitTests_DIR), \
	-L$(LIB_DIR) -lCqrs \
	$(coroutine_LINKER_FLAGS) \
	$(gmock_LINKER_FLAGS)))

endif
