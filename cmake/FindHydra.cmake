######################################################
# - Try to find hdyra (https://www.mpich.org/)
# Once done this will define
#  HYDRA_FOUND - System has hydra
#  HYDRA_INCLUDE_DIRS - The hydra include directories
#  HYDRA_LIBRARIES - The libraries needed to use hydra
#  HYDRA_DEFINITIONS - Compiler switches required for using hydra

######################################################
set(HYDRA_PREFIX "" CACHE STRING "Help cmake to find hydra library (https://www.mpich.org/ into mpich package subdirectory pm/hydra) into your system.")
set(HYDRA_MPIEXEC_NAME "mpiexec" CACHE STRING "Provide the name of mpiexec command from hydra in case you use some prefix/postfix.")

######################################################
find_path(HYDRA_INCLUDE_DIR mpl.h
	HINTS ${HYDRA_PREFIX}/include)

######################################################
find_library(HYDRA_LIBRARY NAMES mpl
	HINTS ${HYDRA_PREFIX}/lib)

######################################################
#extract prefix
get_filename_component(HYDRA_LIB_PREFIX "${HYDRA_LIBRARY}" PATH)
get_filename_component(HYDRA_LIB_PREFIX "${HYDRA_LIB_PREFIX}" PATH)

######################################################
find_program(HYDRA_MPIEXEC ${HYDRA_MPIEXEC_NAME}
	HINTS ${HYDRA_PREFIX}/bin ${HYDRA_LIB_PREFIX}/bin
	NO_DEFAULT_PATH)
	
######################################################
set(HYDRA_LIBRARIES ${HYDRA_LIBRARY} )
set(HYDRA_INCLUDE_DIRS ${HYDRA_INCLUDE_DIR} )

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set HYDRA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(hydra  DEFAULT_MSG
	HYDRA_LIBRARY HYDRA_INCLUDE_DIR HYDRA_MPIEXEC)

######################################################
mark_as_advanced(HYDRA_INCLUDE_DIR HYDRA_LIBRARY HYDRA_MPIEXEC)