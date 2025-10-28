set (USE_KORVO 1)
#set (USE_LOLIN 1)

set(srcs
    main.c
    comms.c
    commandset.c
    )

set(requires
	esp_event
	esp_wifi
	nvs_flash
	spi_flash
	)

if(USE_LOLIN)
set(extra_srcs
    bsp_lolin.c
    )
endif()

if (USE_KORVO)
set(extra_requires
    hardware_driver
    )
endif()    

idf_component_register(INCLUDE_DIRS include
		       SRCS ${srcs} ${extra_srcs}
                       REQUIRES  ${extra_requires}
)

component_compile_options(-w)
