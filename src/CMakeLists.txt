project(telnetd C)

add_executable(${PROJECT_NAME} main.c telnet.c)

if(unity_FOUND)
    include(CTest)
    enable_testing()
    add_executable(${PROJECT_NAME}-test main.c telnet.c)
    add_test(NAME ${PROJECT_NAME}-test COMMAND $<TARGET_FILE:${PROJECT_NAME}-test>)
    target_link_libraries(${PROJECT_NAME}-test PRIVATE unity::unity)
    target_compile_definitions(${PROJECT_NAME}-test PRIVATE TESTING=1)
endif()

