project(RebootInto)
find_package(TCLAP REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)
find_package(Nowide REQUIRED)
if(UNIX)
    find_package(efivar REQUIRED)
endif()

include_directories(
    ${INCLUDE_PATH}
    ${TCLAP_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${Nowide_INCLUDE_DIRS}
    ${efivar_INCLUDE_DIRS}
)

set(SOURCE_FILES
	Platform.h
    UEFI.h
	UEFI.cpp
    main.cpp
)
if(WIN32)
    set(SOURCE_FILES ${SOURCE_FILES}
		Platform_Windows.cpp
    )
endif()
if(UNIX)
    set(SOURCE_FILES ${SOURCE_FILES}
		Platform_Linux.cpp
    )
endif()
source_group("" FILES ${SOURCE_FILES})

if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Ox /Os")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /level='requireAdministrator'")
endif()

add_executable(rebootinto ${SOURCE_FILES})
target_link_libraries(rebootinto
    ${Boost_LIBRARIES}
    ${efivar_LIBRARIES}
)