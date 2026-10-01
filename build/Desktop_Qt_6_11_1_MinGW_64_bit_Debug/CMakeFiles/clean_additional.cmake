# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\boot_upper_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\boot_upper_autogen.dir\\ParseCache.txt"
  "boot_upper_autogen"
  )
endif()
