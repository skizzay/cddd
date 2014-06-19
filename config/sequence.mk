ifndef Sequence_INCLUDE_DIRS
$(eval $(call DEFINE_THIRD_PARTY_LIBRARY, Sequence, $(TOP_DIR)/sequence, -pthread -L/usr/lib/x86_64-linux-gnu -lboost_coroutine -lboost_context))
endif
