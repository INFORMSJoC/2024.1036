cmake_minimum_required (VERSION 3.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../Bapcod/CMake ${CMAKE_CURRENT_SOURCE_DIR}/../../Bapcod/CMake/Packages ${CMAKE_CURRENT_SOURCE_DIR}/CMake ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages)

# this is needed as the demo uses the VRPSolver extension
find_package(BcpRcsp)
find_package(CVRPSEPS)

include(BapcodApplicationProject)
include(BapcodApplicationPackage)
include(Documentation)
include(Profile)

# Optional: Set other libraries
# we include BcpRcsp library as the demo uses the VRPSolver extension
set(USER_LIBRARIES ${BCP_RCSP_LIBRARY} ${CVRPSEPS_LIBRARY})
set(USER_INCLUDE_DIR ${BCP_RCSP_INCLUDE_DIR} ${CVRPSEPS_INCLUDE_DIR})


# Execute the build
set_project_version(1 0 0)

# Build Application
try_build_bapcod_application()

# Make Package
try_build_bapcod_application_package()
