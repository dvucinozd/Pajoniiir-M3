idf_component_register(
    SRCS "app_main.c" "tcm_heap_guard.c"
    INCLUDE_DIRS "."
    REQUIRES p4_ota_pull control_link deck_core bsp_p4_m3 p4_flx4_host library audio_engine audio_recorder ui usb_storage app_settings media_io_gate wifi_link web_server service_log log freertos firmware_health p4_ota

)
