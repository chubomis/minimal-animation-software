find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(CMakeProject1
    main.cpp
    MainWindow.h
    MainWindow.cpp
    CanvasWidget.h
    CanvasWidget.cpp
    Stroke.h
    AppStyle.h
    AppStyle.cpp
)

target_link_libraries(CMakeProject1 PRIVATE Qt6::Widgets)

target_compile_features(CMakeProject1 PRIVATE cxx_std_23)

if(WIN32)
    add_custom_command(TARGET CMakeProject1 POST_BUILD
        COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
                "$<IF:$<CONFIG:Debug>,--debug,--release>"
                "$<TARGET_FILE:CMakeProject1>"
        COMMENT "Deploying Qt runtime DLLs..."
        VERBATIM
    )
endif()