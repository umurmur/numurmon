set(SOURCE_FILES main.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRT} ${CURSES_LIBRARIES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
