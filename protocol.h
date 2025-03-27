// HTTP regex definitions

#pragma once

#define METHOD_REGEX "([a-zA-Z]{1,8})"
#define URI_REGEX "/([a-zA-Z0-9.-]{1,63})"
#define VERSION_REGEX "(HTTP/[0-9].[0-9])"
#define HTTP_VERSION_REGEX "HTTP/1.1"
#define EMPTY_LINE_REGEX "\r\n"
#define REQUEST_LINE_REGEX                                                     \
  METHOD_REGEX " " URI_REGEX " " VERSION_REGEX EMPTY_LINE_REGEX

#define KEY_REGEX "([a-zA-Z0-9.-]{1,128})"
#define VALUE_REGEX "([ -~]{1,128})"
#define HEADER_FIELD_REGEX KEY_REGEX ": " VALUE_REGEX EMPTY_LINE_REGEX

#define MAX_HEADER_LENGTH 2048
