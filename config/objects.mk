get_object_name_from_source=$(abspath $(dir $1))/obj/$(patsubst %.cpp,%.o,$(notdir $(strip $(1))))
get_objects_for_directory=$(foreach src, $(wildcard $(strip $(1))/*.cpp), $(call get_object_name_from_source, $(src)))

define DEFINE_OBJECT_FROM_SOURCE
$(call get_object_name_from_source, $(1)) : $(CONFIG_DIR)/Makefile $(CONFIG_DIR)/objects.mk
$(call get_object_name_from_source, $(1)) : $(abspath $(dir $1))/software.mk
.SECONDEXPANSION:
$(call get_object_name_from_source, $(1)) : $(strip $1)
	@test -d $$(@D) || mkdir -p $$(@D)
	g++-4.9 -c -std=c++1y -g3 -ggdb -Wall -Wextra -Werror -fmessage-length=0 -fPIC -I/usr/include $(addprefix -I,$2) -MMD -MP -MF"$$(@:%.o=%.d)" -MT"$$(@)" -o "$$@" "$(strip $1)"
-include $(patsubst %.o,%.d,$(call get_object_name_from_source, $1))
endef

define DEFINE_OBJECTS_IMPL
$(foreach src,$(wildcard $1/*.cpp), $(eval $(call DEFINE_OBJECT_FROM_SOURCE, $(src), $2)))
CLEAN_OBJECT_DIRS+=$1/obj
endef

# $1 - Source directory
# $2 - Include directories
DEFINE_OBJECTS=$(eval $(call DEFINE_OBJECTS_IMPL, $(strip $1), $2))
