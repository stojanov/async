#pragma once

#include <async/internal/io/pal/io_context.h>
#include <async/internal/io/pal/io_handle.h>
#include <async/internal/io/pal/io_op.h>
#include <async/internal/io/pal/io_type.h>

#if defined(PLATFORM_LINUX)

#include <async/internal/io/platform/linux/io_context.h>
#include <async/internal/io/platform/linux/io_handle.h>
#include <async/internal/io/platform/linux/io_op.h>
#include <async/internal/io/platform/linux/op/read.h>

namespace async::io {
using io_handle = lin::io_handle;
using io_op = lin::io_op;
using io_context = lin::io_context;

using read_op = lin::read_op;
} // namespace async::io

#endif
