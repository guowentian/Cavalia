PROJECT (Cavalia)
file (GLOB SRC_LIST "*.cpp")
ADD_EXECUTABLE (tpcc_benchmark ${SRC_LIST})

TARGET_LINK_LIBRARIES (tpcc_benchmark libjemalloc.so libstorage_engine.so 
	libcityhash.so libpapi.so libnuma.so libprotobuf.so libboost_filesystem.so libboost_system.so libboost_date_time.so libboost_thread.so)
TARGET_LINK_LIBRARIES (tpcc_benchmark pthread rt)
INSTALL (TARGETS tpcc_benchmark
	RUNTIME DESTINATION bin
	)
