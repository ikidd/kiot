project(asdf)
set(PROJECT_VERSION "5.1.90")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.3.0")

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(FeatureSummary)
include(CheckIncludeFiles)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
Core
DBus
Widgets
Mqtt
)

set(KF5_MIN_VERSION "5.2.0")
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Notifications
    IdleTime
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(FOO_SOURCES main.cpp)
add_executable(pc_mqtt_node ${FOO_SOURCES})
target_link_libraries(pc_mqtt_node Qt5::Gui Qt5::Widgets Qt5::Mqtt Qt5::DBus KF5::IdleTime KF5::Notifications)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

