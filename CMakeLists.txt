cmake_minimum_required(VERSION 3.2)
project (exos C CXX ASM_NASM)

set(CMAKE_VERBOSE_MAKEFILE off)

###########################################################
# Generic CMake Utilities

set(SPACE " ")

function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

###########################################################
# Architecture setting. Only i386 is accepted by now.

set(ARCH "i386")

set(RELEASE_BUILD CACHE BOOL "Do a release build if true")
set(USE_GCOV CACHE BOOL "Compile the tests for measuring coverage")

###########################################################

if (${ARCH} STREQUAL "i386")
   set(ARCH_FLAGS "-m32 -march=i686")
   set(NASM_ARCH_FORMAT "elf32")
else()
   message(FATAL_ERROR "Architecture '${ARCH}' not supported.")
endif()


set(BUILD_SCRIPTS ${CMAKE_SOURCE_DIR}/build_scripts)
set(BUILD_SHARED_LIBRARIES off)

# Let's assume the right compiler is set from outside
#set(CMAKE_C_COMPILER ${BUILD_SCRIPTS}/run_gcc)
#set(CMAKE_CXX_COMPILER ${BUILD_SCRIPTS}/run_g++)

set(
   CMAKE_ASM_NASM_COMPILE_OBJECT
   "<CMAKE_ASM_NASM_COMPILER> -f ${NASM_ARCH_FORMAT} -o <OBJECT> <SOURCE>"
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")


#message("CMAKE_C_LINK_EXECUTABLE: ${CMAKE_C_LINK_EXECUTABLE}")
#message("CMAKE_C_COMPILE_OBJECT: ${CMAKE_C_COMPILE_OBJECT}")
#message("CMAKE_CXX_COMPILE_OBJECT: ${CMAKE_CXX_COMPILE_OBJECT}")

set(GENERAL_DEFS_LIST "")

if (${RELEASE_BUILD})

   message(STATUS "Preparing a RELEASE build...")
   LIST(APPEND GENERAL_DEFS_LIST "-DNDEBUG")
   set(OPT_FLAGS_LIST -O2)

else()

   message(STATUS "Preparing a DEBUG build...")
   LIST(APPEND GENERAL_DEFS_LIST "-DDEBUG")
   set(OPT_FLAGS_LIST -O0 -fno-inline-functions)

endif()

set (GCOV_COMPILE_FLAGS "")
set (GCOV_LINK_FLAGS "")

if (${USE_GCOV})
   message(STATUS "Using GCOV for code coverage")
   set(GCOV_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
   set(GCOV_LINK_FLAGS "-fprofile-arcs -lgcov")
endif()


set(
   OTHER_CFLAGS_LIST

   -g
   -fno-stack-protector
)


set(
   WARN_FLAGS_LIST

   -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Werror
)

JOIN("${GENERAL_DEFS_LIST}" ${SPACE} GENERAL_DEFS)
JOIN("${OTHER_CFLAGS_LIST}" ${SPACE} OTHER_CFLAGS)
JOIN("${OPT_FLAGS_LIST}" ${SPACE} OPT_FLAGS)
JOIN("${WARN_FLAGS_LIST}" ${SPACE} WARN_FLAGS)

set(COMMON_FLAGS "${GENERAL_DEFS} ${OTHER_CFLAGS} ${OPT_FLAGS} ${WARN_FLAGS}")

set(BOOTLOADER_BIN_FILE ${CMAKE_BINARY_DIR}/bootloader.bin)
set(KERNEL_BIN_FILE ${CMAKE_BINARY_DIR}/kernel.bin)
set(INIT_FILE ${CMAKE_BINARY_DIR}/init)
set(IMG_FILE ${CMAKE_BINARY_DIR}/exos.img)


################################################################################
# Including sub-directories

add_subdirectory(bootloader)
add_subdirectory(kernel)
add_subdirectory(usermode_init)
add_subdirectory(unittests)

################################################################################

add_custom_command(

   OUTPUT
      ${IMG_FILE}
   COMMAND
      ${BUILD_SCRIPTS}/create_empty_img_if_necessary.sh ${IMG_FILE} 32768
   COMMAND
      dd status=none conv=notrunc if=${BOOTLOADER_BIN_FILE} of=${IMG_FILE} bs=1K
   COMMAND
      dd status=none conv=notrunc if=${KERNEL_BIN_FILE} of=${IMG_FILE} seek=4 bs=1K
   COMMAND
      ${BUILD_SCRIPTS}/build_fatpart.sh ${CMAKE_BINARY_DIR} fatpart
   COMMAND
      # For development purposes, copy just a small part from the fatpart to make the build faster.
      dd status=none conv=notrunc if=${CMAKE_BINARY_DIR}/fatpart of=${IMG_FILE} seek=1024 bs=1K count=512
      #dd status=none conv=notrunc if=${CMAKE_BINARY_DIR}/fatpart of=${IMG_FILE} seek=1024 bs=1K
   DEPENDS
      # We MUST depend on BOTH the wrapper-target AND its OUTPUT in order to this
      # to be re-build when the *.bin files are re-built.
      bootloader kernel usermode_init ${BOOTLOADER_BIN_FILE} ${KERNEL_BIN_FILE} ${INIT_FILE}
   COMMENT
      "Creating the bootable image file"
   VERBATIM
)



add_custom_target(

    exos_image ALL

    DEPENDS
        ${IMG_FILE}
)
