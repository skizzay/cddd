ADD_LIBRARY(cddd_cqrs INTERFACE)
FILE(GLOB cddd_cqrs_headers *.h)
TARGET_SOURCES(cddd_cqrs INTERFACE ${cddd_cqrs_headers})
