include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VENDOR "Dennis Sitelew")
set(CPACK_PACKAGE_CONTACT "yowidin@gmail.com")
set(CPACK_PACKAGE_NAME "CBOR")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)

include(CPack)