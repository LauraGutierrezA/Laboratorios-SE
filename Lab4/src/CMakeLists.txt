# Esto busca todos los archivos .cpp y .h en la carpeta src
FILE(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.h")

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash driver
)