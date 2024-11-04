find_package(emp-ot)
find_package(OpenSSL REQUIRED)

find_package(BLAKE3 REQUIRED)
find_path(BLAKE3_INCLUDE_DIRS NAMES blake3.h)
find_library(BLAKE3_LIBRARIES NAMES libblake3.so PATHS /usr/local/lib REQUIRED)
message(STATUS "BLAKE3_LIBRARIES: ${BLAKE3_LIBRARIES}")

find_path(EMP-ZK_INCLUDE_DIR NAMES emp-zk/emp-zk.h)
find_library(EMP-ZK_LIBRARY NAMES emp-zk)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(aarch64)|(arm64)")
		# M1 Apple
		# find_package(GMP REQUIRED PATHS /opt/homebrew/opt/gmp NO_DEFAULT_PATH)
		set(GMP_DIR /opt/homebrew/opt/gmp)
		ELSE(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(aarch64)|(arm64)")
		# Intel Apple
		set(GMP_DIR /usr/local/opt/gmp)
		# find_package(GMP REQUIRED PATHS /usr/local/opt/gmp NO_DEFAULT_PATH)
	ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "(aarch64)|(arm64)" )
endif()

find_path(GMP_INCLUDE_DIRS gmp.h
    PATHS ${GMP_DIR}/include
    NO_DEFAULT_PATH
)

find_library(GMP_LIBRARIES gmp
    PATHS ${GMP_DIR}/lib
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(emp-zk DEFAULT_MSG EMP-ZK_INCLUDE_DIR EMP-ZK_LIBRARY BLAKE3_LIBRARIES BLAKE3_INCLUDE_DIRS GMP_INCLUDE_DIRS GMP_LIBRARIES)

if(EMP-ZK_FOUND)
	set(EMP-ZK_INCLUDE_DIRS ${EMP-ZK_INCLUDE_DIR} ${EMP-OT_INCLUDE_DIRS} )
	set(EMP-ZK_LIBRARIES ${EMP-OT_LIBRARIES} ${EMP-ZK_LIBRARY} )
endif()
