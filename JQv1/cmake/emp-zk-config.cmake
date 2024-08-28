
find_package(emp-ot)
find_package(OpenSSL REQUIRED)

find_package(BLAKE3 REQUIRED)
find_path(BLAKE3_INCLUDE_DIRS NAMES blake3.h)
find_library(BLAKE3_LIBRARIES NAMES libblake3.so PATHS /usr/local/lib REQUIRED)
message(STATUS "BLAKE3_LIBRARIES: ${BLAKE3_LIBRARIES}")
find_path(EMP-ZK_INCLUDE_DIR NAMES emp-zk/emp-zk.h)
find_library(EMP-ZK_LIBRARY NAMES emp-zk)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(emp-zk DEFAULT_MSG EMP-ZK_INCLUDE_DIR EMP-ZK_LIBRARY BLAKE3_LIBRARIES BLAKE3_INCLUDE_DIRS)

if(EMP-ZK_FOUND)
	set(EMP-ZK_INCLUDE_DIRS ${EMP-ZK_INCLUDE_DIR} ${EMP-OT_INCLUDE_DIRS} )
	set(EMP-ZK_LIBRARIES ${EMP-OT_LIBRARIES} ${EMP-ZK_LIBRARY} )
endif()
