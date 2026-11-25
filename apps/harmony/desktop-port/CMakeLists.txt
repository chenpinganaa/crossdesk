# Compile the desktop application itself. Harmony owns only platform services;
# callbacks, settings, history, DeskHub and session state stay in apps/desktop.
set(DESKTOP "${CROSSDESK_ROOT}/apps/desktop/src")
set(HARMONY "${CROSSDESK_ROOT}/apps/harmony")
file(GLOB RUNTIME CONFIGURE_DEPENDS "${DESKTOP}/gui/runtime/*.cpp")
file(GLOB_RECURSE FEATURES CONFIGURE_DEPENDS "${DESKTOP}/gui/features/*.cpp")
file(GLOB LAN CONFIGURE_DEPENDS "${DESKTOP}/network/lan/*.cpp")
file(GLOB TOOLS CONFIGURE_DEPENDS "${DESKTOP}/tools/*.cpp")
file(GLOB COMMON CONFIGURE_DEPENDS "${DESKTOP}/common/*.cpp" "${DESKTOP}/thumbnail/*.cpp"
    "${DESKTOP}/version_checker/*.cpp" "${CROSSDESK_ROOT}/libs/wire/src/*.cpp")
target_sources(slint_desktop PRIVATE ${RUNTIME} ${FEATURES} ${COMMON} ${LAN} ${TOOLS}
    "${CROSSDESK_ROOT}/libs/controller/src/file_browser.cpp"
    "${DESKTOP}/gui/application/gui_application.cpp"
    "${DESKTOP}/gui/rendering/slint_video_presenter.cpp"
    "${DESKTOP}/platform/common/gui/service_status_runtime.cpp"
    "${DESKTOP}/privacy/privacy_controller.cpp"
    "${DESKTOP}/speaker_capturer/speaker_capture_controller.cpp"
    "${DESKTOP}/config_center/config_center.cpp" "${DESKTOP}/autostart/autostart.cpp"
    "${DESKTOP}/path_manager/path_manager.cpp" "${DESKTOP}/log/rd_log.cpp"
    "${DESKTOP}/tools/file_transfer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/tray.cpp" "${CMAKE_CURRENT_LIST_DIR}/services.cpp" "${CMAKE_CURRENT_LIST_DIR}/devices.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/keyboard_hook.cpp"
    "${HARMONY}/native/input.cpp" "${HARMONY}/native/audio.cpp" "${HARMONY}/native/capture.cpp"
    "${HARMONY}/native/capture_surface.cpp" "${HARMONY}/native/clipboard.cpp")
target_include_directories(slint_desktop PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}" "${HARMONY}/native" "${DESKTOP}" "${DESKTOP}/gui"
    "${DESKTOP}/common" "${DESKTOP}/config_center" "${DESKTOP}/autostart"
    "${DESKTOP}/path_manager" "${DESKTOP}/log" "${DESKTOP}/tools"
    "${DESKTOP}/thumbnail" "${DESKTOP}/version_checker" "${DESKTOP}/privacy"
    "${DESKTOP}/device_controller" "${DESKTOP}/speaker_capturer" "${DESKTOP}/screen_capturer"
    "${DESKTOP}/platform/common/input" "${CROSSDESK_ROOT}/libs/wire/include"
    "${CROSSDESK_ROOT}/libs/controller/include"
    "${CROSSDESK_ROOT}/deps/submodules/minirtc/src/api"
    "${CROSSDESK_ROOT}/deps/submodules/minirtc/src/common"
    "${CROSSDESK_ROOT}/deps/submodules/minirtc/src/ws"
    "${CROSSDESK_ROOT}/deps/submodules/minirtc/src/pc"
    "${CROSSDESK_ROOT}/deps/submodules/minirtc/src/media/resolution_adapter")
# xmake caches a host (x86_64) build of every package next to the target one, and
# even installs an "openssl" 1.1.1 alias for python. Linking that set would mix
# architectures, so packages are selected by the arch recorded in manifest.txt.
file(GLOB PACKAGE_MANIFESTS "${HARMONY}/.native/packages/*/*/*/*/manifest.txt")
set(PACKAGE_INCLUDES "")
set(PACKAGE_LIBS "")
foreach(MANIFEST ${PACKAGE_MANIFESTS})
    file(STRINGS "${MANIFEST}" MANIFEST_ARCH REGEX "^ *arch = ")
    if(NOT MANIFEST_ARCH MATCHES "\"arm64-v8a\"")
        continue()
    endif()
    get_filename_component(PACKAGE_ROOT "${MANIFEST}" DIRECTORY)
    if(PACKAGE_ROOT MATCHES "/packages/o/openssl/")
        continue()
    endif()
    if(IS_DIRECTORY "${PACKAGE_ROOT}/include")
        list(APPEND PACKAGE_INCLUDES "${PACKAGE_ROOT}/include")
    endif()
    file(GLOB PACKAGE_ARCHIVES "${PACKAGE_ROOT}/lib/*.a")
    list(APPEND PACKAGE_LIBS ${PACKAGE_ARCHIVES})
endforeach()
target_include_directories(slint_desktop SYSTEM PRIVATE ${PACKAGE_INCLUDES})
file(GLOB QUEUE_INCLUDE "${HARMONY}/.native/packages/c/concurrentqueue/*/*/include/concurrentqueue/moodycamel")
target_include_directories(slint_desktop SYSTEM PRIVATE ${QUEUE_INCLUDE} "${SLINT_DESKTOP_DEPS}")
# websocketpp defaults to Boost.Asio; the desktop target builds against the
# standalone Asio package, exactly as the xmake gui target does.
target_compile_definitions(slint_desktop PRIVATE FMT_CONSTEVAL= CPPHTTPLIB_OPENSSL_SUPPORT
    ASIO_STANDALONE CROSSDESK_VERSION="${CROSSDESK_VERSION}" CROSSDESK_HARMONY_DESKTOP=1)
# Use exactly the native dependency versions built by scripts/build.py.
set(NATIVE_LIB "${HARMONY}/.native/build/harmony/arm64-v8a/release/libminirtc.a")
if(NOT EXISTS "${NATIVE_LIB}")
    message(FATAL_ERROR "Build apps/harmony/scripts/build.py --native-only first")
endif()
target_link_libraries(slint_desktop PRIVATE "-Wl,--start-group" "${NATIVE_LIB}"
    ${PACKAGE_LIBS} "-Wl,--end-group" native_buffer native_image ohaudio native_avscreen_capture
    native_media_core native_media_codecbase native_media_vdec ohinput pasteboard udmf)
target_link_options(slint_desktop PRIVATE -Wl,--no-undefined)
