#pragma once

namespace async::io::pal {
// support edge trigger cases
enum class io_type { IN, OUT, HUP, ERR };
} // namespace async::io::pal
