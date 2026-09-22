file(GLOB_RECURSE scrld ./*.c)
idf_component_register(
  SRCS ${scrld}
  REQUIRES lvgl
  INCLUDE_DIRS "page_calendar" "fonts" "page_wifi_setup" "page_tarot")

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
