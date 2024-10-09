include_directories(..)
add_library(KDEHAIntegrations OBJECT
    components.cpp
    login1_manager_interface.cpp
    scripts.cpp
    shortcuts.cpp
)

target_link_libraries(KDEHAIntegrations
    KF6::IdleTime
    KF6::GlobalAccel
    KF6::Notifications
    KF6::ConfigCore
    Qt6::Gui
    Qt6::Widgets
    Qt6::Mqtt
    Qt6::DBus
)
