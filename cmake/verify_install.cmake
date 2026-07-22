if(NOT DEFINED SZHBOOKS_INSTALL_ROOT OR SZHBOOKS_INSTALL_ROOT STREQUAL "")
    message(FATAL_ERROR "SZHBOOKS_INSTALL_ROOT is required")
endif()

file(TO_CMAKE_PATH "${SZHBOOKS_INSTALL_ROOT}" install_root)

set(required_files
    "SZHBooks.exe"
    "Qt6Core.dll"
    "Qt6Gui.dll"
    "Qt6Qml.dll"
    "Qt6Quick.dll"
    "msvcp140.dll"
    "vcruntime140.dll"
    "vcruntime140_1.dll"
    "plugins/platforms/qwindows.dll"
    "plugins/sqldrivers/qsqlite.dll"
    "release-manifest.json"
    "docs/README.md"
    "docs/RELEASE_NOTES.md"
    "docs/THIRD_PARTY_NOTICES.md"
    "docs/licenses/Qt-LICENSE.txt"
    "docs/licenses/miniz-LICENSE.txt"
)

foreach(relative_path IN LISTS required_files)
    if(NOT EXISTS "${install_root}/${relative_path}")
        message(FATAL_ERROR "Installed package is missing ${relative_path}")
    endif()
endforeach()

file(SIZE "${install_root}/SZHBooks.exe" executable_size)
if(executable_size LESS 100000)
    message(FATAL_ERROR "Installed SZHBooks.exe is unexpectedly small")
endif()

file(READ "${install_root}/release-manifest.json" release_manifest)
string(JSON manifest_version GET "${release_manifest}" version)
string(JSON manifest_portable GET "${release_manifest}" portable)

if(DEFINED SZHBOOKS_EXPECTED_VERSION
   AND NOT manifest_version STREQUAL SZHBOOKS_EXPECTED_VERSION)
    message(FATAL_ERROR
        "Expected version ${SZHBOOKS_EXPECTED_VERSION}, found ${manifest_version}")
endif()

if(SZHBOOKS_EXPECT_PORTABLE)
    if(NOT EXISTS "${install_root}/portable.flag")
        message(FATAL_ERROR "Portable install is missing portable.flag")
    endif()
    if(NOT manifest_portable)
        message(FATAL_ERROR "Portable release manifest is marked as installed")
    endif()
else()
    if(EXISTS "${install_root}/portable.flag")
        message(FATAL_ERROR "Installed package unexpectedly contains portable.flag")
    endif()
    if(manifest_portable)
        message(FATAL_ERROR "Installed release manifest is marked as portable")
    endif()
endif()

message(STATUS
    "Verified SZHBooks ${manifest_version} install tree at ${install_root}")
