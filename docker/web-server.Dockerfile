ARG BASE_IMAGE=ghcr.io/games-on-whales/base:edge

####################################
# Build web client
FROM node:20-slim AS base-web
ENV PNPM_HOME="/pnpm"
ENV PATH="$PNPM_HOME:$PATH"
RUN corepack enable
COPY src/server/web-client /app
WORKDIR /app

FROM base-web AS build-web
RUN --mount=type=cache,id=pnpm,target=/pnpm/store pnpm install --frozen-lockfile
RUN pnpm run build

####################################
# Build inputtino server
FROM $BASE_IMAGE as build-server

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
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_STANDARD=20 \
    -DBUILD_SERVER=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DBoost_USE_STATIC_LIBS=ON \
    -G Ninja && \
    ninja -C $CMAKE_BUILD_DIR input-server && \
    # We have to copy out the built executables because this will only be available inside the buildkit cache
    cp $CMAKE_BUILD_DIR/src/server/input-server /inputtino/input-server



####################################
# Final image
FROM $BASE_IMAGE

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
    libevdev2 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /inputtino

COPY --from=build-web /app/dist /inputtino/dist
COPY --from=build-server /inputtino/input-server /inputtino/input-server

ENV INPUTTINO_CLIENT_PATH=/inputtino/dist \
    INPUTTINO_REST_IP="0.0.0.0" \
    INPUTTINO_REST_PORT=8080 \
    GOW_REQUIRED_DEVICES="/dev/uinput /dev/input/event*" \
    XDG_RUNTIME_DIR=/tmp

EXPOSE 8080

COPY --chmod=777 docker/startup.sh /opt/gow/startup.sh
