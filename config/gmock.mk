ifndef gmock_INCLUDE_DIRS
$(eval $(call DEFINE_THIRD_PARTY_LIBRARY, gmock, /usr/src/gmock/include /usr/src/gmock/gtest/include, $(call use_archive, $(HOME)/third-party/gmock/libgmock.a $(HOME)/third-party/gmock/gtest/libgtest.a)))
endif
