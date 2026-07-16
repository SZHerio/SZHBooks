qt_add_executable(appSZHBooks
    MANUAL_FINALIZATION
    "${PROJECT_SOURCE_DIR}/main.cpp"
)

# Defining the executable in a dedicated source scope keeps qmlimportscanner
# focused on the application's resource files instead of repository-wide tests.
set_target_properties(appSZHBooks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>"
)
