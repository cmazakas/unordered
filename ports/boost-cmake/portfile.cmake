set(VCPKG_POLICY_CMAKE_HELPER_PORT enabled)

include("${CMAKE_CURRENT_LIST_DIR}/ref_sha.cmake")

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH_SUPER
    REPO boostorg/boost
    REF ${boost_boost_ref}
    SHA512 ${boost_boost_sha512}
    HEAD_REF master
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH_CMAKE
    REPO cmazakas/cmake
    REF 09daf623c5919db8ea385c17e0191fecdb4f4a26
    SHA512 283ff9d5be5cccb3a05cb09681415651d3afa511917b33d53f54c9f50286fddf3debd72a043cf3857de362799e1aa1d220bbad8e495192f8ed695f7bdaa59913
    HEAD_REF vcpkg-test
    PATCHES
      "vcpkg-build.diff"
      "fix-mpi.diff"
      "no-prefix.diff"
      "no-config-suffix.diff"
      "no-honor-static.diff"
      "add-optional-deps.diff"
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.in" "${SOURCE_PATH_CMAKE}/CMakeLists.txt" @ONLY)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH_CMAKE}")
vcpkg_cmake_install()

file(INSTALL "${SOURCE_PATH_CMAKE}/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/share/boost/cmake-build")

file(INSTALL
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-port-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH_SUPER}/LICENSE_1_0.txt")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
