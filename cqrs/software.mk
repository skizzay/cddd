ifndef Cqrs_DIR
Cqrs_DIR:=$(call get_software_dir)

include $(CONFIG_DIR)/sequence.mk

$(call DEFINE_OBJECTS, $(Cqrs_DIR), $(abspath $(Cqrs_DIR)/../..))
$(eval $(call DEFINE_SHARED_LIBRARY, Cqrs, $(Cqrs_DIR), boost_context boost_system boost_coroutine))
$(eval $(call DEFINE_SHARED_LIBRARY_LOCATIONS, Cqrs, /usr/lib/x86_64-linux-gnu))

endif
