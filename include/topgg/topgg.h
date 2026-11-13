#pragma once

#if defined(_WIN32) && !defined(_WINSOCKAPI_)
#define _WINSOCKAPI_
#endif

#include <topgg/debug.h>
#include <topgg/models.h>
#include <topgg/util.h>
#ifdef TOPGG_WEBHOOKS
#include <topgg/webhooks/client.h>
#include <topgg/webhooks/models.h>
#endif
#include <topgg/client.h>
#include <topgg/exception.h>
#include <topgg/http.h>
#include <topgg/result.h>