cmake_minimum_required(VERSION 3.26) # el9
project(lab3)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

add_compile_options(-ggdb3)
add_link_options(-fuse-ld=mold)

find_package(LLVM 20.1 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

find_program(ALIVE_TV alive-tv REQUIRED)

add_library(Lab3 SHARED
    lib/UnitLICM.cpp
    lib/UnitLoopInfo.cpp
    lib/RegisterPasses.cpp
    lib/UnitSCCP.cpp
)
target_include_directories(Lab3 PUBLIC include)
target_compile_definitions(Lab3 PRIVATE
    $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
)
target_compile_options(Lab3 PRIVATE
    $<$<CONFIG:Debug>:-Og>
)
target_precompile_headers(Lab3 PRIVATE
    <cassert>
    <utility>
    <iterator>
    <vector>
    <functional>
    <algorithm>
    <ranges>
    <optional>
    <llvm/Support/Casting.h>
    <llvm/Support/raw_ostream.h>
    <llvm/Support/Debug.h>
    <llvm/ADT/ArrayRef.h>
    <llvm/ADT/STLExtras.h>
    <llvm/ADT/SmallVector.h>
    <llvm/ADT/SmallPtrSet.h>
    <llvm/ADT/SmallSet.h>
    <llvm/ADT/DenseMap.h>
    <llvm/ADT/GraphTraits.h>
    <llvm/ADT/ilist_iterator.h>
    <llvm/ADT/iterator_range.h>
    <llvm/ADT/Statistic.h>
    <llvm/IR/Value.h>
    <llvm/IR/Use.h>
    <llvm/IR/User.h>
    <llvm/IR/Constant.h>
    <llvm/IR/Instruction.h>
    <llvm/IR/Function.h>
    <llvm/IR/BasicBlock.h>
    <llvm/IR/PatternMatch.h>
    <llvm/IR/Instructions.h>
    <llvm/IR/InstIterator.h>
    <llvm/IR/Constants.h>
    <llvm/IR/Dominators.h>
    <llvm/IR/PassManager.h>
    <llvm/Analysis/AliasAnalysis.h>
    <llvm/Analysis/AliasSetTracker.h>
    <llvm/Analysis/ValueTracking.h>
)

include(CTest)
enable_testing()

file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/test/*.ll)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test)

foreach (test ${tests})
    get_filename_component(file ${test} NAME_WLE)
    add_test(
        NAME "opt.licm.${file}"
            COMMAND ${LLVM_BINARY_DIR}/bin/opt
            --load-pass-plugin=$<TARGET_FILE:Lab3>
            -passes=function\(mem2reg,instcombine<no-verify-fixpoint>,simplifycfg,adce\),inline,globaldce,function\(sroa,early-cse,jump-threading,correlated-propagation,simplifycfg,instcombine<no-verify-fixpoint>,tailcallelim,simplifycfg,reassociate,unit-licm,adce,simplifycfg,instcombine<no-verify-fixpoint>\),globaldce
            -debug-only=unit-licm,unit-loop
            --verify-each
            ${test} -S
            -o "${CMAKE_BINARY_DIR}/test/${file}.opt.licm.ll"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    )
    set_tests_properties("opt.licm.${file}" PROPERTIES
        FIXTURES_SETUP "${test}.opt.licm"
        LABELS "LICM"
    )
    # alive-tv src (original) tgt (optimized)
    add_test(
        NAME "alive.licm.${file}"
        COMMAND ${ALIVE_TV} ${test} "${CMAKE_BINARY_DIR}/test/${file}.opt.licm.ll"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    )
    set_tests_properties("alive.licm.${file}" PROPERTIES
        FIXTURES_REQUIRED "${test}.opt.licm"
        LABELS "LICM"
    )

    add_test(
        NAME "opt.sccp.${file}"
        COMMAND ${LLVM_BINARY_DIR}/bin/opt
            --load-pass-plugin=$<TARGET_FILE:Lab3>
            -passes=function\(mem2reg,instcombine<no-verify-fixpoint>,simplifycfg,adce\),inline,globaldce,function\(sroa,early-cse,unit-sccp,jump-threading,correlated-propagation,simplifycfg,instcombine<no-verify-fixpoint>,tailcallelim,simplifycfg,reassociate,unit-licm,adce,simplifycfg,instcombine<no-verify-fixpoint>\),globaldce
            #-passes="function(mem2reg,instcombine,simplifycfg,adce),inline,globaldce,function(sroa,early-cse,unit-sccp,jump-threading,correlated-propagation,simplifycfg,instcombine,simplifycfg,reassociate,unit-licm,adce,simplifycfg,instcombine),globaldce"
            -debug-only=unit-sccp,unit-loop
            --verify-each
            ${test} -S -o "${CMAKE_BINARY_DIR}/test/${file}.opt.sccp.ll"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    )
    set_tests_properties("opt.sccp.${file}" PROPERTIES
        FIXTURES_SETUP "${test}.opt.sccp"
        LABELS "SCCP"
    )
    # alive-tv src (original) tgt (optimized)
    add_test(
        NAME "alive.sccp.${file}"
        COMMAND ${ALIVE_TV} ${test} "${CMAKE_BINARY_DIR}/test/${file}.opt.sccp.ll"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    )
    set_tests_properties("alive.sccp.${file}" PROPERTIES
        FIXTURES_REQUIRED "${test}.opt.sccp"
        LABELS "SCCP"
    )
endforeach ()

