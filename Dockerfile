# Build stage
FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY cmake/ ./cmake/
COPY man/ ./man/
COPY src/ ./src/

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DGRIDOT_ENABLE_WERROR=ON \
        -DCMAKE_INSTALL_PREFIX=/usr \
    && cmake --build build \
    && cmake --install build

# Runtime stage
FROM debian:bookworm-slim AS runtime

# `apt` is required: gridot reads /var/lib/apt/extended_states and shells out
# to apt-mark to work out which packages were auto-installed.
RUN apt-get update \
    && apt-get install -y --no-install-recommends dpkg apt \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/bin/gridot /usr/bin/gridot
COPY --from=build /usr/share/man/man1/gridot.1 /usr/share/man/man1/gridot.1

ENV XDG_CONFIG_HOME=/data

ENTRYPOINT ["/usr/bin/gridot"]
CMD ["scan"]

LABEL org.opencontainers.image.title="gridot" \
      org.opencontainers.image.description="Get Rid of That - report unused packages" \
      org.opencontainers.image.source="https://github.com/codersilicon/gridot" \
      org.opencontainers.image.licenses="Apache-2.0"
