# ----------------------------
# BUILD STAGE
# ----------------------------
FROM debian:stable-slim AS builder

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip pkg-config \
    bison flex perl zlib1g-dev autoconf automake libtool

COPY vcpkg.json vcpkg-configuration.json ./
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_DEFAULT_BINARY_CACHE=/app/vcpkg-cache
RUN mkdir -p /app/vcpkg-cache

RUN ./vcpkg/vcpkg install

COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_TOOLCHAIN_FILE=/app/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make -j$(nproc)

# ----------------------------
# RUNTIME STAGE
# ----------------------------
FROM debian:stable-slim AS runtime

WORKDIR /app

# Install only runtime libraries (example: libpq)
RUN apt-get update && apt-get install -y \
    libpq5 openssl curl && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy compiled binary
COPY --from=builder /app/build/dogs_api .

# Make sure it's executable
RUN chmod +x dogs_api

# Expose port
EXPOSE 8080

# Start the server
CMD ["./dogs_api"]