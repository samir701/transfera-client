# Builder stage
FROM gcc:14 AS builder

# Install minimal build deps and clean apt caches
RUN apt-get update && \
    apt-get install -y --no-install-recommends cmake build-essential ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Leverage Docker cache: copy project files needed for configure first
COPY CMakeLists.txt CMakeLists.txt
COPY include/ include/

# Copy rest of source (after configure files so cache can be reused)
COPY src/ src/

# Configure and build (Release). CMakeLists.txt has no install rules, so copy the binary from build/
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc) && \
    strip build/server || true

# Runtime stage
FROM gcc:14

# Install only runtime deps and clean apt lists
RUN apt-get update && \
    apt-get install -y --no-install-recommends libstdc++6 ca-certificates curl && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root user
# why: Running as non-root enhances security by limiting the potential damage if the application is compromised. It prevents unauthorized access to system files and reduces the risk of privilege escalation attacks.
# how: The useradd command creates a new system user named "appuser" with a home directory and sets the shell to /usr/sbin/nologin, which prevents the user from logging in interactively. This ensures that the application runs with limited permissions, adhering to the principle of least privilege.
# reference: https://docs.docker.com/develop/develop-images/dockerfile_best-practices/#user
RUN useradd --system --create-home --shell /usr/sbin/nologin appuser

# Copy built binary from builder's build/ dir
COPY --from=builder /src/build/server /usr/local/bin/server

# Set ownership & working dir
RUN chown appuser:appuser /usr/local/bin/server
WORKDIR /app
USER appuser


# Ensure the app uses port 8008 inside the container to match runtime port mapping
ENV PORT=8008
EXPOSE 8008

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s CMD curl -f http://127.0.0.1:8008/api/health || exit 1

ENTRYPOINT ["/usr/local/bin/server"]