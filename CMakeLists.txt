cmake_minimum_required(VERSION 3.19)
project(boot_upper LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort)

qt_standard_project_setup()

qt_add_executable(boot_upper
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    iapcodec.cpp
    iapcodec.h
    iapclient.cpp
    iapclient.h
)

target_link_libraries(boot_upper
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::SerialPort
)

if (MSVC)
    target_compile_options(boot_upper PRIVATE /utf-8)
else()
    target_compile_options(boot_upper PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

install(TARGETS boot_upper
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET boot_upper
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
