#pragma once

/// USER_SECTION_START 1

/// USER_SECTION_END

// Debugging
#ifdef NDEBUG
#define CLC_CONSOLE(msg)
#define CLC_CONSOLE_FUNCTION(msg)
#else
#include <iostream>

#define CLC_DEBUG
#define CLC_CONSOLE_STREAM std::cout

#define CLC_CONSOLE(msg) CLC_CONSOLE_STREAM << msg;
#define CLC_CONSOLE_FUNCTION(msg) CLC_CONSOLE_STREAM << __PRETTY_FUNCTION__ << " " << msg;
#endif

/// USER_SECTION_START 2

/// USER_SECTION_END

#ifdef CLC_PROFILING
#include "easy/profiler.h"
#include <easy/arbitrary_value.h> // EASY_VALUE, EASY_ARRAY are defined here

#define CLC_PROFILING_BLOCK_C(text, color) EASY_BLOCK(text, color)
#define CLC_PROFILING_NONSCOPED_BLOCK_C(text, color) EASY_NONSCOPED_BLOCK(text, color)
#define CLC_PROFILING_END_BLOCK EASY_END_BLOCK
#define CLC_PROFILING_FUNCTION_C(color) EASY_FUNCTION(color)
#define CLC_PROFILING_BLOCK(text, colorStage) CLC_PROFILING_BLOCK_C(text,profiler::colors::  colorStage)
#define CLC_PROFILING_NONSCOPED_BLOCK(text, colorStage) CLC_PROFILING_NONSCOPED_BLOCK_C(text,profiler::colors::  colorStage)
#define CLC_PROFILING_FUNCTION(colorStage) CLC_PROFILING_FUNCTION_C(profiler::colors:: colorStage)
#define CLC_PROFILING_THREAD(name) EASY_THREAD(name)

#define CLC_PROFILING_VALUE(name, value) EASY_VALUE(name, value)
#define CLC_PROFILING_TEXT(name, value) EASY_TEXT(name, value)

#else
#define CLC_PROFILING_BLOCK_C(text, color)
#define CLC_PROFILING_NONSCOPED_BLOCK_C(text, color)
#define CLC_PROFILING_END_BLOCK
#define CLC_PROFILING_FUNCTION_C(color)
#define CLC_PROFILING_BLOCK(text, colorStage)
#define CLC_PROFILING_NONSCOPED_BLOCK(text, colorStage)
#define CLC_PROFILING_FUNCTION(colorStage)
#define CLC_PROFILING_THREAD(name)

#define CLC_PROFILING_VALUE(name, value)
#define CLC_PROFILING_TEXT(name, value)
#endif

// Special expantion tecniques are required to combine the color name
#define CONCAT_SYMBOLS_IMPL(x, y) x##y
#define CONCAT_SYMBOLS(x, y) CONCAT_SYMBOLS_IMPL(x, y)



// Different color stages
#define CLC_COLOR_STAGE_1 50
#define CLC_COLOR_STAGE_2 100
#define CLC_COLOR_STAGE_3 200
#define CLC_COLOR_STAGE_4 300
#define CLC_COLOR_STAGE_5 400
#define CLC_COLOR_STAGE_6 500
#define CLC_COLOR_STAGE_7 600
#define CLC_COLOR_STAGE_8 700
#define CLC_COLOR_STAGE_9 800
#define CLC_COLOR_STAGE_10 900
#define CLC_COLOR_STAGE_11 A100 
#define CLC_COLOR_STAGE_12 A200 
#define CLC_COLOR_STAGE_13 A400 
#define CLC_COLOR_STAGE_14 A700 


// General
#define CLC_GENERAL_PROFILING_COLORBASE Cyan
#define CLC_GENERAL_PROFILING_BLOCK_C(text, color) CLC_PROFILING_BLOCK_C(text, color)
#define CLC_GENERAL_PROFILING_NONSCOPED_BLOCK_C(text, color) CLC_PROFILING_NONSCOPED_BLOCK_C(text, color)
#define CLC_GENERAL_PROFILING_END_BLOCK CLC_PROFILING_END_BLOCK;
#define CLC_GENERAL_PROFILING_FUNCTION_C(color) CLC_PROFILING_FUNCTION_C(color)
#define CLC_GENERAL_PROFILING_BLOCK(text, colorStage) CLC_PROFILING_BLOCK(text, CONCAT_SYMBOLS(CLC_GENERAL_PROFILING_COLORBASE, colorStage))
#define CLC_GENERAL_PROFILING_NONSCOPED_BLOCK(text, colorStage) CLC_PROFILING_NONSCOPED_BLOCK(text, CONCAT_SYMBOLS(CLC_GENERAL_PROFILING_COLORBASE, colorStage))
#define CLC_GENERAL_PROFILING_FUNCTION(colorStage) CLC_PROFILING_FUNCTION(CONCAT_SYMBOLS(CLC_GENERAL_PROFILING_COLORBASE, colorStage))
#define CLC_GENERAL_PROFILING_VALUE(name, value) CLC_PROFILING_VALUE(name, value)
#define CLC_GENERAL_PROFILING_TEXT(name, value) CLC_PROFILING_TEXT(name, value)


/// USER_SECTION_START 3

/// USER_SECTION_END
