cmake_minimum_required(VERSION 3.16)
project(RSA_Lab3_GUI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(wxWidgets_USE_STATIC 1)

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(RSA_Lab3_GUI WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
)

target_include_directories(RSA_Lab3_GUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(RSA_Lab3_GUI PRIVATE ${wxWidgets_LIBRARIES})

if(APPLE)
    set_target_properties(RSA_Lab3_GUI PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME      "RSA Lab3"
        MACOSX_BUNDLE_BUNDLE_VERSION   "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )

    install(TARGETS RSA_Lab3_GUI BUNDLE DESTINATION .)

    # Фиксируем пути к dylib внутри .app, чтобы работало без Homebrew
    find_program(DYLIBBUNDLER dylibbundler)
    if(DYLIBBUNDLER)
        install(CODE "
            execute_process(COMMAND ${DYLIBBUNDLER}
                -od -b -x \"\${CMAKE_INSTALL_PREFIX}/RSA_Lab3_GUI.app/Contents/MacOS/RSA_Lab3_GUI\"
                -d \"\${CMAKE_INSTALL_PREFIX}/RSA_Lab3_GUI.app/Contents/libs\"
                -p @executable_path/../libs)
        ")
    endif()

    set(CPACK_GENERATOR              "DragNDrop")
    set(CPACK_PACKAGE_NAME           "RSA_Lab3")
    set(CPACK_PACKAGE_VERSION        "1.0.0")
    set(CPACK_DMG_VOLUME_NAME        "RSA Lab 3")
    set(CPACK_PACKAGE_FILE_NAME      "RSA_Lab3")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "") # чистый макет
    include(CPack)
endif()
