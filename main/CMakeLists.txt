file(GLOB STATUS_LIGHTS_SRCS
    "status_lights/*.cpp"
    "status_lights/transitions/*.cpp"
    "status_lights/animations/*.cpp"
    "commands/*.cpp"
)

set(STATUS_LIGHTS_INCLUDE_DIRS
    "."
    "status_lights"
    "status_lights/transitions"
    "status_lights/animations"
    "commands"
)

idf_component_register(
    SRCS "main.cpp" ${STATUS_LIGHTS_SRCS}
    INCLUDE_DIRS ${STATUS_LIGHTS_INCLUDE_DIRS}
)

