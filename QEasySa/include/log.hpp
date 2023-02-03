#ifndef __HRG_LOG__
#define __HRG_LOG__
#pragma execution_character_set("utf-8")
#define SPDLOG_TRACE_ON
#define SPDLOG_DEBUG_ON
#define SPDLOG_INFO_ON
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <iostream>
#include <string>
#include <sstream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/logger.h"
#include "spdlog/async.h"

#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/common.h"

using namespace spdlog;

#endif