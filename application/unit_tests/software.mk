ifndef unit_tests_DIR
unit_tests_DIR:=$(call get_software_dir)

include $(CONFIG_DIR)/gmock.mk
include $(TOP_DIR)/cqrs/test/unit/software.mk

$(call DEFINE_OBJECTS, $(unit_tests_DIR), $(gmock_INCLUDE_DIRS) $(TOP_DIR) $(Sequence_INCLUDE_DIRS))
$(eval $(call DEFINE_EXECUTABLE, unit_tests, $(unit_tests_DIR)))

$(eval $(call DEFINE_EXECUTABLE_TO_SHARED_DEPENDENCIES, unit_tests, Cqrs))
$(eval $(call DEFINE_EXECUTABLE_TO_WHOLE_ARCHIVE_DEPENDENCY, unit_tests, CqrsUnitTests))

endif
