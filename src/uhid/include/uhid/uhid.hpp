#pragma once

#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <inputtino/result.hpp>
#include <linux/uhid.h>
#include <memory>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace uhid {
struct ThreadState {
  int fd;
  std::function<void(const uhid_event &ev, int fd)> on_event;
  bool stop_repeat_thread = false;
};

struct Device {
  std::thread ev_thread;
  std::shared_ptr<ThreadState> state;
};

struct DeviceDefinition {
  std::string name;
  std::string phys;
  std::string uniq;
  uint16_t bus;
  uint32_t vendor;
  uint32_t product;
  uint32_t version;
  uint32_t country;
  std::vector<unsigned char> report_description;
};

static inputtino::Result<bool> uhid_write(int fd, const struct uhid_event *ev) {
  ssize_t ret = write(fd, ev, sizeof(*ev));
  if (ret < 0) {
    return inputtino::Error(strerror(errno));
  } else if (ret != sizeof(*ev)) {
    return inputtino::Error(strerror(-EFAULT));
  } else {
    return ret;
  }
}

static void destroy(Device *dev) {
  struct uhid_event ev {};
  ev.type = UHID_DESTROY;
  uhid_write(dev->state->fd, &ev);

  close(dev->state->fd); // This should also close the thread by causing a POLLHUP

  dev->state->stop_repeat_thread = true;
  if (dev->ev_thread.joinable()) {
    dev->ev_thread.join(); // let's wait for the thread to finish
  }
  delete dev;
}

static void set_c_str(const std::string &str, unsigned char *c_str) {
  std::copy(str.begin(), str.end(), c_str);
  c_str[str.length()] = 0;
}

static inputtino::Result<std::shared_ptr<Device>>
create(const DeviceDefinition &definition, const std::function<void(const uhid_event &ev, int fd)> &on_event) {

  int fd = open("/dev/uhid", O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    return inputtino::Error(strerror(errno));
  }

  auto ev = uhid_event{};
  ev.type = UHID_CREATE2, ev.u.create2.bus = definition.bus, ev.u.create2.vendor = definition.vendor,
  ev.u.create2.product = definition.product, ev.u.create2.version = definition.version,
  ev.u.create2.country = definition.country,
  ev.u.create2.rd_size = static_cast<__u16>(definition.report_description.size()),
  std::copy(definition.report_description.begin(), definition.report_description.end(), ev.u.create2.rd_data);
  set_c_str(definition.name, ev.u.create2.name);
  set_c_str(definition.phys, ev.u.create2.phys);
  set_c_str(definition.uniq, ev.u.create2.uniq);

  auto res = uhid_write(fd, &ev);
  if (res) {
    auto state = std::make_shared<ThreadState>();
    state->on_event = on_event;
    state->fd = fd;
    auto device = std::shared_ptr<Device>(new Device{.state = state}, destroy);
    auto thread = std::thread([state]() {
      ssize_t ret;
      struct pollfd pfds[1];
      pfds[0].fd = state->fd;
      pfds[0].events = POLLIN;

      while (!state->stop_repeat_thread) {
        ret = poll(pfds, 1, -1);
        if (ret < 0) {
          fprintf(stderr, "Cannot poll for fds: %m\n");
          break;
        }
        if (pfds[0].revents & POLLHUP) {
          fprintf(stderr, "Received HUP on uhid-cdev\n");
          break;
        }
        if (pfds[0].revents & POLLIN) {
          struct uhid_event ev {};
          ret = read(state->fd, &ev, sizeof(ev));
          if (ret == 0) {
            fprintf(stderr, "Read HUP on uhid-cdev\n");
          } else if (ret < 0) {
            fprintf(stderr, "Cannot read uhid-cdev: %m\n");
          } else if (ret != sizeof(ev)) {
            fprintf(stderr, "Invalid size read from uhid-dev: %zd != %zu\n", ret, sizeof(ev));
          } else {
            state->on_event(ev, state->fd);
          }
        }
      }
    });
    thread.detach();
    device->ev_thread = std::move(thread);
    return device;
  } else {
    close(fd);
    return inputtino::Error(res.getErrorMessage());
  }
}

} // namespace uhid