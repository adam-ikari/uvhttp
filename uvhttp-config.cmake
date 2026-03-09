
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was uvhttp-config.in.cmake                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# Find dependencies
find_dependency(Libuv REQUIRED)
if(BUILD_WITH_WEBSOCKET OR BUILD_WITH_HTTPS)
    find_dependency(MbedTLS REQUIRED)
endif()
find_dependency(xxhash REQUIRED)

# Include exported targets
include("${CMAKE_CURRENT_LIST_DIR}/uvhttp-targets.cmake")

# Set package variables
set(UVHTTP_VERSION 2.4.0)
set(UVHTTP_INCLUDE_DIRS "/usr/local/include")
set(UVHTTP_LIBRARIES uvhttp)
set(UVHTTP_LIBRARY_DIRS "/usr/local/lib")

check_required_components(uvhttp)

# Feature flags
set(UVHTTP_FEATURE_WEBSOCKET OFF)
set(UVHTTP_FEATURE_HTTPS OFF)
set(UVHTTP_FEATURE_STATIC_FILES )
set(UVHTTP_FEATURE_RATE_LIMIT )
set(UVHTTP_FEATURE_LRU_CACHE )
set(UVHTTP_FEATURE_ROUTER_CACHE )
set(UVHTTP_FEATURE_LOGGING )
