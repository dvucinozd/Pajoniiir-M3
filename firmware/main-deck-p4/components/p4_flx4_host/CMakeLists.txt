idf_component_register(
    SRCS "p4_flx4_map.c"
         "p4_flx4_led.c"
         "p4_flx4_uac.c"
         "p4_flx4_midi_gate.c"
         "p4_flx4_host.c"
    INCLUDE_DIRS "include"
    REQUIRES log freertos control_link esp_timer usb
)
