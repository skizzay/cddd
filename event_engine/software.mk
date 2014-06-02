ifndef EventEngine_DIR
EventEngine_DIR:=$(call get_software_dir)

$(call DEFINE_OBJECTS, $(EventEngine_DIR), $(abspath $(EventEngine_DIR)/../..))
$(eval $(call DEFINE_SHARED_LIBRARY, EventEngine, $(EventEngine_DIR)))

endif
