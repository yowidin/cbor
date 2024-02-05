include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CBOR_VERSION_CONFIG "${CBOR_GENERATED_CMAKE_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(CBOR_PROJECT_CONFIG "${CBOR_GENERATED_CMAKE_DIR}/${PROJECT_NAME}Config.cmake")

set(CBOR_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(CBOR_TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(CBOR_INSTALL_NAMESPACE "${PROJECT_NAME}::")

# TODO: Change to SameMajorVersion when API becomes stable enough
write_basic_package_version_file("${CBOR_VERSION_CONFIG}" COMPATIBILITY ExactVersion)
configure_package_config_file(
    cmake/Config.cmake.in
    "${CBOR_PROJECT_CONFIG}"
    INSTALL_DESTINATION "${CBOR_INSTALL_CMAKE_DIR}"
)

set(CBOR_INSTALL_TARGETS cbor)

install(
    TARGETS ${CBOR_INSTALL_TARGETS}

    EXPORT ${CBOR_TARGETS_EXPORT_NAME}

    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT CBOR_Runtime

    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT CBOR_Runtime
    NAMELINK_COMPONENT CBOR_Development

    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT CBOR_Development

    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CBOR_GENERATED_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
    FILES
    ${CBOR_GENERATED_EXPORT_HEADER}
    DESTINATION
    "${CMAKE_INSTALL_INCLUDEDIR}/cbor"
)

install(
    FILES
    ${CBOR_PROJECT_CONFIG}
    ${CBOR_VERSION_CONFIG}
    DESTINATION
    ${CBOR_INSTALL_CMAKE_DIR}
)

set_target_properties(cbor PROPERTIES EXPORT_NAME library)
add_library(CBOR::library ALIAS cbor)

install(
    EXPORT ${CBOR_TARGETS_EXPORT_NAME}
    DESTINATION ${CBOR_INSTALL_CMAKE_DIR}
    NAMESPACE ${CBOR_INSTALL_NAMESPACE}
    COMPONENT CBOR_Development
)