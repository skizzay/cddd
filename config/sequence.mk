ifndef Sequence_INCLUDE_DIRS
$(call DEFINE_THIRD_PARTY_LIBRARY, $(TOP_DIR)/sequence, -pthread -L/usr/lib/x86_64-linux-gnu -lboost_coroutine -lboost_context)
endif
