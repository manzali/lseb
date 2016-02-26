######################################################
#Search for deps
find_package(Hydra REQUIRED)

######################################################
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckStructHasMember)
#check for config file
CHECK_INCLUDE_FILES (assert.h HAVE_ASSERT_H)
CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILES (netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILES (netinet/tcp.h HAVE_NETINET_TCP_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (sys/param.h HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILES (sys/param.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (sys/un.h HAVE_SYS_UN_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
#check attributes
CHECK_C_SOURCE_COMPILES("int main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {}" HAVE_GCC_ATTRIBUTE)
#symbols
CHECK_FUNCTION_EXISTS(gethostbyname HAVE_GETHOSTBYNAME) 
CHECK_FUNCTION_EXISTS(setsockopt HAVE_SETSOCKOPT)
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
CHECK_FUNCTION_EXISTS(strncasecmp HAVE_STRNCASECMP)
#members
CHECK_STRUCT_HAS_MEMBER("struct hostent" "h_addr_list" "netdb.h" HAVE_H_ADDR_LIST)
#set some vars
set(PACKAGE_BUGREPORT "")
set(PACKAGE_NAME "")
set(PACKAGE_STRING "")
set(PACKAGE_TARNAME "")
set(PACKAGE_URL "")
set(PACKAGE_VERSION "")
set(STDC_HEADERS yes)
set(USE_PMI_PORT yes)


######################################################
#setup config file
configure_file(pmiconfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/pmiconfig.h" @ONLY)

######################################################
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${HYDRA_INCLUDE_DIR})
add_library(hydra-simple simple_pmi.c simple_pmiutil.c safestr2.c)