ifndef Messaging_DIR
Messaging_DIR:=$(call get_software_dir)

$(call DEFINE_OBJECTS, $(Messaging_DIR), $(abspath $(Messaging_DIR)/../..))
$(eval $(call DEFINE_SHARED_LIBRARY, Messaging, $(Messaging_DIR)))

endif
