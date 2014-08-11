ifndef coroutine_LINKER_FLAGS
$(eval $(call DEFINE_THIRD_PARTY_LIBRARY, coroutine, , -L/usr/lib/x86_64-linux-gnu -lboost_context -lboost_system /usr/lib/x86_64-linux-gnu/libboost_coroutine.a))
endif
