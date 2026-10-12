cmake_minimum_required(VERSION 3.22)
add_library(box2d STATIC)

target_include_directories(box2d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        # So BOX2D_USER_CONFIG="box2d/Config.h" resolves to ArcEngine's overrides.
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include>
)

target_sources(box2d PRIVATE
    src/aabb.c
    src/arena_allocator.c
    src/bitset.c
    src/body.c
    src/broad_phase.c
    src/constraint_graph.c
    src/contact.c
    src/contact_solver.c
    src/contact_solver_w4.c
    src/contact_solver_w8.c
    src/core.c
    src/distance.c
    src/distance_joint.c
    src/dynamic_tree.c
    src/geometry.c
    src/hull.c
    src/id_pool.c
    src/island.c
    src/joint.c
    src/manifold.c
    src/math_functions.c
    src/motor_joint.c
    src/mover.c
    src/mover_joint.c
    src/parallel_for.c
    src/physics_world.c
    src/pogo_joint.c
    src/prismatic_joint.c
    src/recording.c
    src/replay.c
    src/revolute_joint.c
    src/scheduler.c
    src/sensor.c
    src/shape.c
    src/simd.c
    src/snapshot.c
    src/solver.c
    src/solver_set.c
    src/table.c
    src/timer.c
    src/types.c
    src/weld_joint.c
    src/wheel_joint.c
)

target_compile_features(box2d PRIVATE c_std_17)
target_compile_options(box2d PRIVATE -fno-unwind-tables -fno-exceptions)

# ArcEngine ships its own Box2D overrides (float precision, W4 solver) in
# engine/include/box2d/Config.h. Point the library at them via BOX2D_USER_CONFIG
# so the lib and every consumer compile with the SAME config — otherwise the
# app could see a different struct layout/solver than the compiled lib (ABI skew).
target_compile_definitions(box2d PUBLIC
    BOX2D_USER_CONFIG="box2d/Config.h"
)

message(STATUS "Box2D vendor: ${CMAKE_CURRENT_SOURCE_DIR}")
