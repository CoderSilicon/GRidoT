# Build stage
FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt CMakePresets.json ./
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

RUN apt-get update \
    && apt-get install -y --no-install-recommends dpkg \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/bin/gridot /usr/bin/gridot
COPY --from=build /usr/share/man/man1/gridot.1 /usr/share/man/man1/gridot.1

ENV XDG_DATA_HOME=/data

ENTRYPOINT ["/usr/bin/gridot"]
CMD ["scan"]

LABEL org.opencontainers.image.title="gridot" \
      org.opencontainers.image.description="Get Rid of That - report unused Debian/Ubuntu packages" \
      org.opencontainers.image.source="https://github.com/gridot/gridot" \
      org.opencontainers.image.licenses="MIT"