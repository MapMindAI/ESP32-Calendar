idf_component_register(
    SRCS "user_app.cpp" "driver_update.cpp" "button_interface.cpp"
    PRIV_REQUIRES main ui_bsp
    INCLUDE_DIRS "./")
