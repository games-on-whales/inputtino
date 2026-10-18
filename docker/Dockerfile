ARG BASE_IMAGE=ghcr.io/games-on-whales/base:fedora-44

####################################
FROM $BASE_IMAGE AS build-libinputtino

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    ccache \
    ninja-build \
    cmake \
    clang \
    pkg-config \
    git \
    libevdev-dev \
    && rm -rf /var/lib/apt/lists/*


COPY . /inputtino/
WORKDIR /inputtino

ENV CCACHE_DIR=/cache/ccache
ENV CMAKE_BUILD_DIR=/cache/cmake-build
RUN --mount=type=cache,target=/cache/ccache \
    cmake -B$CMAKE_BUILD_DIR \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_C_BINDINGS=ON \
    -DLIBINPUTTINO_INSTALL=ON \
    -DBUILD_SHARED_LIBS=ON \
    -G Ninja && \
    ninja -C $CMAKE_BUILD_DIR install

####################################
FROM $BASE_IMAGE AS base-libinputtino

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends libevdev2 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build-libinputtino /usr/include/inputtino /usr/include/inputtino
COPY --from=build-libinputtino /usr/share/pkgconfig/libinputtino.pc /usr/share/pkgconfig/libinputtino.pc
COPY --from=build-libinputtino /usr/lib/x86_64-linux-gnu/liblibinputtino* /usr/lib/x86_64-linux-gnu/

####################################
FROM base-libinputtino AS build-rust-bindings

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    pkg-config \
    build-essential \
    clang \
    curl \
    && curl https://sh.rustup.rs -sSf | bash -s -- -y \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="${HOME}/.cargo/bin:${PATH}"

COPY ./bindings/rust /inputtino-rust
WORKDIR /inputtino-rust


RUN cargo build --release

