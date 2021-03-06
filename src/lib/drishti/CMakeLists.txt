# Copyright (c) 2014-2017, David Hirvonen
# All rights reserved.

include(drishti_symbol_list)
include(drishti_strip)
include(drishti_split_debug_symbols)

option(DRISHTI_PRINT_ALL "Print all variables" OFF)
if(DRISHTI_PRINT_ALL)
  include(drishti_print_env_var)
  drishti_print_env_var()
endif()

## Customize linker flags
include(CheckCCompilerFlag)
if(NOT MSVC)
  check_c_compiler_flag("-Wl,-dead_strip" FLAG_dead_strip)
  if(NOT FLAG_dead_strip)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
  else()
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
  endif()
endif()

set(LIB_TYPE STATIC)

if(DRISHTI_BUILD_FACE)
  hunter_add_package(nlohmann_json)
  find_package(nlohmann_json CONFIG REQUIRED)
endif()

##################
#### world #######
##################

set(DRISHTI_WORLD_SOURCES
  ${DRISHTI_CORE_SRCS}
  ${DRISHTI_CORE_HDRS_PUBLIC}
  ${DRISHTI_GEOMETRY_SRCS}
  ${DRISHTI_GEOMETRY_HDRS_PUBLIC}
  ${DRISHTI_SENSOR_SRCS}
  ${DRISHTI_SENSOR_HDRS_PUBLIC}
  ${DRISHTI_ML_SRCS}
  ${DRISHTI_ML_HDRS_PUBLIC}
  ${DRISHTI_RCPR_SRCS}
  ${DRISHTI_RCPR_HDRS_PUBLIC}
  ${DRISHTI_EYE_SRCS}
  ${DRISHTI_EYE_HDRS_PUBLIC}
  ${DRISHTI_GRAPHICS_SRCS}
  ${DRISHTI_GRAPHICS_HDRS_PUBLIC}
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Here we provide our own IDE source groupings according to namespace
# Each source_group() will be a subfolder of the drishti_world target:

### core
source_group("core\\Header Files" FILES ${DRISHTI_CORE_HDRS_PUBLIC})
source_group("core\\Source Files" FILES ${DRISHTI_CORE_SRCS})

### geometry
source_group("geometry\\Header Files" FILES ${DRISHTI_GEOMETRY_HDRS_PUBLIC})
source_group("geometry\\Source Files" FILES ${DRISHTI_GEOMETRY_SRCS})

### sensor
source_group("sensor\\Header Files" FILES ${DRISHTI_SENSOR_HDRS_PUBLIC})
source_group("sensor\\Source Files" FILES ${DRISHTI_SENSOR_SRCS})

### ml
source_group("ml\\Header Files" FILES ${DRISHTI_ML_HDRS_PUBLIC})
source_group("ml\\Source Files" FILES ${DRISHTI_ML_SRCS})

### rcpr
source_group("rcpr\\Header Files" FILES ${DRISHTI_RCPR_HDRS_PUBLIC})
source_group("rcpr\\Source Files" FILES ${DRISHTI_RCPR_SRCS})

### eye
source_group("eye\\Header Files" FILES ${DRISHTI_EYE_HDRS_PUBLIC})
source_group("eye\\Source Files" FILES ${DRISHTI_EYE_SRCS})

### graphics
source_group("graphics\\Header Files" FILES ${DRISHTI_GRAPHICS_HDRS_PUBLIC})
source_group("graphics\\Source Files" FILES ${DRISHTI_GRAPHICS_SRCS})

if(DRISHTI_BUILD_FACE)
  ### face
  source_group("face\\Header Files" FILES ${DRISHTI_FACE_HDRS_PUBLIC})
  source_group("face\\Source Files" FILES ${DRISHTI_FACE_SRCS})
  list(APPEND DRISHTI_WORLD_SOURCES ${DRISHTI_FACE_SRCS} ${DRISHTI_FACE_HDRS_PUBLIC})
endif()

if(DRISHTI_BUILD_OGLES_GPGPU)
  list(APPEND DRISHTI_SDK_3RDPARTY_LIBS ${OGLES_GPGPU_LIB})

  ### hci
  # currently requires ogles_gpgpu
  # TODO: needs CPU only path
  if(DRISHTI_BUILD_HCI AND DRISHTI_BUILD_OGLES_GPGPU)
    source_group("hci\\Header Files" FILES ${DRISHTI_HCI_HDRS_PUBLIC})
    source_group("hci\\Source Files" FILES ${DRISHTI_HCI_SRCS})
    list(APPEND DRISHTI_WORLD_SOURCES ${DRISHTI_HCI_SRCS} ${DRISHTI_HCI_HDRS_PUBLIC})
  endif()

endif()

source_group("drishti\\Header Files Private" FILES ${DRISHTI_DRISHTI_HDRS_PRIVATE})
source_group("drishti\\Header Files" FILES ${DRISHTI_DRISHTI_HDRS_PUBLIC})
source_group("drishti\\Source Files" FILES ${DRISHTI_DRISHTI_SRCS})

add_library(drishti_world ${LIB_TYPE} ${DRISHTI_WORLD_SOURCES})
set_property(TARGET drishti_world PROPERTY FOLDER "libs/drishti")

target_link_libraries(drishti_world PUBLIC ${DRISHTI_SDK_3RDPARTY_LIBS} xgboost::xgboost Eigen3::Eigen)
if(DRISHTI_BUILD_FACE)
  target_link_libraries(drishti_world PUBLIC nlohmann_json)
endif()

if(DRISHTI_USE_THREAD_POOL_CPP)
  target_link_libraries(drishti_world PUBLIC thread-pool-cpp::thread-pool-cpp)
endif()

if(DRISHTI_COTIRE)
  cotire(drishti_world)
  set(drishti_libs drishti_world_unity)
  set(drishti_regression_libs drishti_world_unity)
  set(drishti_half_libs drishti_world_unity)
else()
  set(drishti_libs drishti_world)
  set(drishti_regression_libs drishti_world)
  set(drishti_half_libs drishti_world)
endif()

# For Android platforms, we must specify -DHALF_ENABLE_CPP11_CMATH=0 for all modules
# using the half precision floating point storage (i.e., package half)
if(ANDROID)
  foreach(library ${drishti_half_libs})
    target_compile_definitions(${library} PUBLIC HALF_ENABLE_CPP11_CMATH=0)
  endforeach()
endif()

# For all regression modules, we must provide definitions corresponding
# to options for ensemble of regression tree leaf node accumulation:
#   -DDRISHTI_BUILD_REGRESSION_SIMD=(0|1)
#   -DDRISHTI_BUILD_REGRESSION_FIXED_POINT=(0|1)
foreach(library ${drishti_regression_libs})
  drishti_bool_to_int(DRISHTI_BUILD_REGRESSION_SIMD build_regression_simd)
  target_compile_definitions(${library} PUBLIC DRISHTI_BUILD_REGRESSION_SIMD=${build_regression_simd})
  drishti_bool_to_int(DRISHTI_BUILD_REGRESSION_FIXED_POINT build_regression_fixed_point)
  target_compile_definitions(${library} PUBLIC DRISHTI_BUILD_REGRESSION_FIXED_POINT=${build_regression_fixed_point})
endforeach()

# Build and install a single library or framework from our set of "object" libraries
# Note: Due to complications with object libraries we are either using per module
# static libraries or a single drishti_world compilation from all global siources
# gathered by the Sugar package.
#   * https://public.kitware.com/Bug/bug_relationship_graph.php?bug_id=15038&graph=dependency
#   * http://www.cmake.org/pipermail/cmake/2014-February/057055.html
#   * https://public.kitware.com/Bug/bug_relationship_graph.php?bug_id=14970&graph=relation
set(drishti_sdk_srcs master/drishti_master.hpp master/drishti_master.cpp)

# Create dependency list
set(drishti_sdk_libs ${drishti_libs} ${DRISHTI_SDK_3RDPARTY_LIBS})

if(${DRISHTI_DO_GPU_TESTING} AND MSVC)
  # FaceTracker.cpp -> FaceTracker.hpp -> Context.hpp -> drishti_gl.hpp -> gl/glew.h
  hunter_add_package(glew)
  find_package(glew CONFIG REQUIRED)
endif()

### drishtisdk (static)
add_library(drishtisdk ${drishti_sdk_srcs})
target_link_libraries(drishtisdk PUBLIC ${drishti_sdk_libs})

# support private access to drishti/drishti_sdk.hpp
target_include_directories(drishtisdk
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

if(DRISHTI_BUILD_SHARED_SDK)
  drishti_strip(drishtisdk)
endif()

set_target_properties(drishtisdk
  PROPERTIES
  PUBLIC_HEADER "${DRISHTISDK_PUBLIC_HDRS}"
)

if(NOT IOS)
  set_target_properties(drishtisdk
    PROPERTIES
    SOVERSION "${drishtisdk_VERSION_MAJOR}"
    VERSION "${drishtisdk_VERSION}"
  )
endif()

# Set the serialization mode preprocessor flags (boost and/or cereal)
set(DRISHTI_LIBRARY_LIST ${drishti_libs} drishtisdk)
foreach(library ${DRISHTI_LIBRARY_LIST})

  target_include_directories(${library}
    PUBLIC
    "$<BUILD_INTERFACE:${DRISHTI_INCLUDE_DIRECTORIES}>"
    "$<INSTALL_INTERFACE:${drishti_include_install_dir}>"
  )

  if(DRISHTI_OPENGL_ES3)
    target_compile_definitions(${library} PUBLIC DRISHTI_OPENGL_ES3=1)
  endif()

  # -DDRISHTI_BUILD_MIN_SIZE=(0|1)
  drishti_bool_to_int(DRISHTI_BUILD_MIN_SIZE build_min_size)
  target_compile_definitions(${library} PUBLIC DRISHTI_BUILD_MIN_SIZE=${build_min_size})

  message("DRISHTI_BUILD_MIN_SIZE=${build_min_size} ${DRISHTI_BUILD_MIN_SIZE}")

  # define M_PI_2 for MSVC
  target_compile_definitions(${library} PUBLIC _USE_MATH_DEFINES)

  if(DRISHTI_SERIALIZE_WITH_CVMATIO)
    target_compile_definitions(${library} PUBLIC DRISHTI_SERIALIZE_WITH_CVMATIO=1)
  endif()

  if(DRISHTI_BUILD_OGLES_GPGPU)
    target_compile_definitions(${library} PUBLIC DRISHTI_BUILD_OGLES_GPGPU=1)
  endif()

endforeach()

############################
###### PUBLIC API ##########
############################

if(DRISHTI_COPY_3RDPARTY_LICENSES)
  # Install license files
  include(drishti_copy_3rdparty_licenses)
  drishti_copy_3rdparty_licenses("3rdparty/licenses")
endif()

#################################################
### Generate the main shared public interface ###
#################################################

# keep header file version strings up to date:
set(drishti_sdk_hpp_in "${CMAKE_CURRENT_LIST_DIR}/drishti/drishti_sdk.hpp.in")
set(drishti_sdk_hpp "${CMAKE_CURRENT_BINARY_DIR}/drishti/drishti_sdk.hpp")
configure_file("${drishti_sdk_hpp_in}" "${drishti_sdk_hpp}" @ONLY)

# The public interface can be built as either SHARED or STATIC.
# There are two classes of end users: 1) hunter and 2) non-hunter.
# For hunter users, either SHARED OR STATIC targets can be used
# transparently.  For non-hunter users, SHARED libraries will
# provide dependency free use automatically, or the
# DRISHTI_MERGE_ALL_LIBS option can be used to provide a single
# static library (all dependencies are identified, split and
# merged into a single monolithic library for convenient dependency
# free linking).

if(DRISHTI_BUILD_SHARED_SDK)
  set(api_lib_type SHARED)
else()
  set(api_lib_type STATIC)
endif()

if((NOT DRISHTI_BUILD_SHARED_SDK) AND DRISHTI_BUILD_MERGED_SDK)
  set(drishti_api_name drishti_stage) # create a temporary target
else()
  set(drishti_api_name drishti)
endif()

# These targets will be configured with appropriate include paths:
set(drishti_interface_libs "")

include (GenerateExportHeader)
set(drishti_export_header "${CMAKE_CURRENT_BINARY_DIR}/drishti/drishti_export.hpp")

# Requires:
# @DRISHTI_SDK_MAX_FACES@
set(DRISHTI_SDK_MAX_FACES 8 CACHE STRING "Maximum # of faces to be supported")
message("DRISHTI_SDK_MAX_FACES ${DRISHTI_SDK_MAX_FACES}")
set(drishti_constants_hpp_in "${CMAKE_CURRENT_LIST_DIR}/drishti/drishti_constants.hpp.in")
set(drishti_constants_hpp "${CMAKE_CURRENT_BINARY_DIR}/drishti/drishti_constants.hpp")
configure_file("${drishti_constants_hpp_in}" "${drishti_constants_hpp}" @ONLY)

set(DRISHTI_INSTALL_PUBLIC_HDRS
  ${DRISHTI_DRISHTI_HDRS_PUBLIC}
  "${drishti_export_header}"
  "${drishti_constants_hpp}"
  "${drishti_sdk_hpp}"
)

# Create the public shared library
add_library(${drishti_api_name} ${api_lib_type}
  ${DRISHTI_INSTALL_PUBLIC_HDRS}
  ${DRISHTI_DRISHTI_SRCS}
  ${DRISHTI_DRISHTI_HDRS_PRIVATE})
target_compile_definitions(${drishti_api_name} PUBLIC _USE_MATH_DEFINES) # define M_PI_2 for Visual Studio
target_link_libraries(${drishti_api_name} PRIVATE "${drishti_sdk_libs}")
list(APPEND drishti_interface_libs ${drishti_api_name})

# Append drishti subdirectory for installed headers
set(drishti_include_install_dir "${drishti_include_install_dir}/${drishti_api_name}")

if(DRISHTI_BUILD_SHARED_SDK)
  if(IOS)
    # Configure Info.plist template:
    #  @CF_BUNDLE_SHORT_VERSION_STRING@
    #  @CF_BUNDLE_VERSION@
    #  @CF_BUNDLE_NAME@
    #  @CF_BUNDLE_IDENTIFIER@
    set(CF_BUNDLE_SHORT_VERSION_STRING "${drishtisdk_VERSION}")
    set(CF_BUNDLE_VERSION "${drishtisdk_VERSION}")
    set(CF_BUNDLE_NAME "drishti")
    set(CF_BUNDLE_IDENTIFIER "com.elucideye.drishti")
    configure_file(
      "${DRISHTI_TOP_DIR}/cmake/Modules/templates/Info.plist.in"
      "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
      @ONLY
    )

    # https://cmake.org/cmake/help/latest/prop_tgt/FRAMEWORK.html#framework
    set_target_properties(${drishti_api_name} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER "${CF_BUNDLE_IDENTIFIER}"
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
      VERSION "${drishtisdk_VERSION}" # "current version" in semantic format in Mach-O binary file
      SOVERSION "${drishtisdk_VERSION}" # "compatibility version" in semantic format in Mach-O binary file
      PUBLIC_HEADER "${DRISHTI_INSTALL_PUBLIC_HDRS}"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iOS Developer"
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone/iPad
      INSTALL_NAME_DIR "@rpath//"
      BUILD_WITH_INSTALL_NAME_DIR YES
    )

    if(DRISHTI_CODE_SIGN)
      set_target_properties(${drishti_api_name} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
      )
    else()
      set_target_properties(${drishti_api_name} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
      )
    endif()
  endif()
elseif(DRISHTI_BUILD_MERGED_SDK)
  # (Optional) merge top level library and all dependencies:
  # Also see: http://stackoverflow.com/a/18949281
  include(drishti_merge_libraries)
  drishti_merge_libraries(LIBRARIES ${drishti_api_name} FINAL drishti)
  list(APPEND drishti_interface_libs drishti)
endif()

generate_export_header(drishti
  EXPORT_FILE_NAME "${drishti_export_header}"
)

# ALIAS target allow us to have same interface while including project
add_library(drishti::drishti ALIAS drishti)

if(DRISHTI_BUILD_SHARED_SDK AND NOT (IOS AND DRISHTI_BUILD_MERGED_SDK))
  # Error: ld: Assertion failed: (0 && "need to handle arm64 -r reloc")
  #drishti_strip(drishti)
endif()

if(DRISHTI_BUILD_SHARED_SDK)
  drishti_split_debug_symbols(drishti)
endif()

###################################
### set build/install include paths
###################################

if(NOT IOS)
  # Use standrad shared lib versioning except for Xcode shared frameworks
  set_target_properties(drishti
    PROPERTIES
    VERSION "${drishtisdk_VERSION}"
    SOVERSION "${drishtisdk_VERSION_MAJOR}"
  )
endif()

foreach(library ${drishti_libs} drishtisdk drishti)
  set_property(TARGET ${library} PROPERTY FOLDER "libs/drishti")
endforeach()

if(DRISHTI_HAVE_TO_STRING)
  foreach(library ${drishti_libs} drishtisdk drishti)
    target_compile_definitions(${library} PUBLIC $<BUILD_INTERFACE:DRISHTI_HAVE_TO_STRING>)
  endforeach()
endif()

###################
##### install #####
###################

if(DRISHTI_INSTALL)

  # See: https://cmake.org/Wiki/CMake/Tutorials/How_to_create_a_ProjectConfig.cmake_file
  if(DRISHTI_BUILD_SHARED_SDK)
    set(DRISHTI_INSTALL_TARGETS "")
  else()
    set(DRISHTI_INSTALL_TARGETS ${drishti_libs})
  endif()

  # Export minimal public SDK or full internal SDK
  set(TARGET_SDK "drishti")

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file("${drishti_version_config}"
    VERSION "${drishtisdk_VERSION}"
    COMPATIBILITY SameMajorVersion
    )

  # Note: variable 'drishti_targets_export_name' used
  # TODO: Belongs to top level CMakeLists.txt
  configure_file(
      "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
      "${drishti_project_config}"
      @ONLY
  )

  install(
    TARGETS ${TARGET_SDK} ${DRISHTI_INSTALL_TARGETS}
    EXPORT "${drishti_targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    FRAMEWORK DESTINATION "."
    INCLUDES DESTINATION "${drishti_include_install_dir}"
  )

  if(DRISHTI_BUILD_SHARED_SDK AND IOS)
    # Headers are part of framework
  else()
    install(
      FILES
      ${DRISHTI_INSTALL_PUBLIC_HDRS}
      DESTINATION "${drishti_include_install_dir}"
    )
  endif()

  install(
    FILES "${drishti_project_config}" "${drishti_version_config}"
    DESTINATION "${drishti_config_install_dir}"
  )

  install(
    EXPORT "${drishti_targets_export_name}"
    NAMESPACE "${drishti_namespace}"
    DESTINATION "${drishti_config_install_dir}"
  )
endif()

# Experimental size reduction

##################
##### C API ######
##################

if(DRISHTI_BUILD_C_INTERFACE)

  if((NOT DRISHTI_BUILD_SHARED_SDK) AND DRISHTI_BUILD_MERGED_SDK)
    set(drishti_c_api_name drishti_c_stage) # create a temporary target
  else()
    set(drishti_c_api_name drishti_c)
  endif()

  add_library(${drishti_c_api_name} ${api_lib_type} ${DRISHTI_DRISHTI_SRCS} ${DRISHTI_DRISHTI_HDRS_PUBLIC})
  set_property(TARGET ${drishti_c_api_name} PROPERTY FOLDER "libs/drishti")
  target_compile_definitions(${drishti_c_api_name} PUBLIC _USE_MATH_DEFINES) # define M_PI_2 for Visual Studio
  target_link_libraries(${drishti_c_api_name} PRIVATE "${drishti_sdk_libs}")
  list(APPEND drishti_interface_libs ${drishti_c_api_name})

  if((NOT DRISHTI_BUILD_SHARED_SDK) AND DRISHTI_BUILD_MERGED_SDK)
    # (Optional) merge top level library and all dependencies:
    # Also see: http://stackoverflow.com/a/18949281
    include(drishti_merge_libraries)
    drishti_merge_libraries(LIBRARIES ${drishti_c_api_name} FINAL drishti_c)
    list(APPEND drishti_interface_libs drishti_c)
  else()
    # Only use symbol_list and version script for non drishti_merge_libraries scenarios:
    # Undefined symbols for architecture x86_64:
    # "_drishti_eye_segmenter_create_from_file", referenced from:
    drishti_symbol_list(drishti_c)
  endif()

  if(DRISHTI_BUILD_SHARED_SDK AND NOT (IOS AND DRISHTI_BUILD_MERGED_SDK))
    # Error: ld: Assertion failed: (0 && "need to handle arm64 -r reloc")
    #drishti_strip(drishti_c)
  endif()

  if(DRISHTI_INSTALL)
    # Simple install to execute post build strip
    install(
      TARGETS drishti_c
      EXPORT "${drishti_targets_export_name}"
      LIBRARY DESTINATION "lib"
      ARCHIVE DESTINATION "lib"
      RUNTIME DESTINATION "bin"
      INCLUDES DESTINATION "${drishti_include_install_dir}"
      )
  endif()
endif()

foreach(library ${drishti_interface_libs})
  target_include_directories(${library}
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include>"  # <prefix>/include
  )
endforeach()

#################
## Unit tests ###
#################

# NOTE: These should come last to ensure all library targets are available
if(DRISHTI_BUILD_TESTS)

  if(IOS)
    # Add a static public SDK library:
    # * support CTest/GTest executables on iOS until CMake dynamic framework embedding is sorted out
    # * use this library for IDE debugging as an alternative to the stripped public library
    add_library(drishti_static STATIC ${DRISHTI_DRISHTI_SRCS} ${DRISHTI_DRISHTI_HDRS_PUBLIC})
    target_compile_definitions(drishti_static PUBLIC _USE_MATH_DEFINES) # define M_PI_2 for Visual Studio
    target_link_libraries(drishti_static PRIVATE "${drishti_sdk_libs}")
    set_property(TARGET drishti_static PROPERTY FOLDER "libs/drishti")
    target_include_directories(drishti_static
      PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>"
      "$<INSTALL_INTERFACE:include>"  # <prefix>/include
    )
  endif()

endif()
