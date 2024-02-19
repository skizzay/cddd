add_library(gtd_project
        main/business/model/project/events/project_created.h
        main/business/model/project/events/project_added_task.h
        main/business/model/project/events/project_completed.h
        main/business/model/project/project.h
        main/business/model/project/project.cpp
        main/business/model/project/commands/create_project.h
        main/business/model/project/commands/add_task_to_project.h
        main/business/model/project/commands/cancel_project.h
        main/business/model/project/events/project_reactivated.h
        main/business/model/project/commands/mark_project_task_as_completed.h
        main/business/model/project/events/project_task_completed.h
        main/business/model/project/commands/remove_task_from_project.h
        main/business/model/project/commands/reactivate_task_within_project.h
        main/business/model/project/events/project_removed_task.h
        main/business/model/project/events/project_task_reactivated.h
        main/business/model/project/events/project_event.h
        main/business/io_utils.h
)
set_property(TARGET gtd_project PROPERTY CXX_STANDARD 23)
target_link_libraries(gtd_project PUBLIC
        simple_cqrs
        simple_s11n
)
target_include_directories(gtd_project PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/gtd/main>
        $<INSTALL_INTERFACE:include>
)


find_package(Catch2 REQUIRED)
include(Catch)
add_executable(gtd_project_test test/business/model/project/command_processing.t.cpp)

set_property(TARGET gtd_project_test PROPERTY CXX_STANDARD 23)
target_link_libraries(gtd_project_test PUBLIC
        Catch2::Catch2WithMain
        gtd_project
)

catch_discover_tests(gtd_project_test)