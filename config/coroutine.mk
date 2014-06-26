ifndef coroutine_LINKER_FLAGS
$(eval $(call DEFINE_THIRD_PARTY_LIBRARY, coroutine, , $(foreach archive, coroutine context system, $(call use_archive, /usr/lib/x86_64-linux-gnu/libboost_$(archive).a))))
endif
