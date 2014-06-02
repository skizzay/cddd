ifndef Cqrs_DIR
Cqrs_DIR:=$(call get_software_dir)

$(call DEFINE_OBJECTS, $(Cqrs_DIR), $(abspath $(Cqrs_DIR)/../..))
$(eval $(call DEFINE_SHARED_LIBRARY, Cqrs, $(Cqrs_DIR)))

endif
