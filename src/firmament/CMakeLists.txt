add_library(execution execution.cpp)
add_executable(firmament firmament.cpp)
target_link_libraries(firmament execution ${LIBS})
