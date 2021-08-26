/**
 * Copyright 2021 nosoop
 * 
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "logging.h"

#include <cstdarg>
#include <cstdio>

#include "convar.h"

#include "tier0/dbg.h"

static CBotLogger s_Logger;
CBotLogger *logger = &s_Logger;

ConVar rcbot_loglevel("rcbot_loglevel", "2", 0, "Display logging messages with increasing verbosity (higher number = more messages)");

const char* LOGLEVEL_STRINGS[] = {
	"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

void CBotLogger::Log(LogLevel level, const char* fmt, ...) {
	if (level > static_cast<LogLevel>(rcbot_loglevel.GetInt())) {
		return;
	}
	
	char buf[1024];
	
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr); 
	va_end(argptr);
	
	if (level <= LogLevel::WARN) {
		Warning("[RCBot] %s: %s\n", LOGLEVEL_STRINGS[level], buf);
	} else {
		Msg("[RCBot] %s: %s\n", LOGLEVEL_STRINGS[level], buf);
	}
}
