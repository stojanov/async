#include <async/io/pal/io_handle.h>

namespace async::io::lin {

class io_handle : public pal::io_handle {
  public:
    io_handle(int fd) : _fd(fd) {}

    std::any native() const override { return _fd; }

    std::size_t hash_code() const override {
        static std::hash<int> hasher;
        return hasher(_fd);
    }

  private:
    int _fd;
};
} // namespace async::io::lin
