FROM archlinux:latest

LABEL org.opencontainers.image.description="An image used for building koality projects"

# Update and install build tools and compilers
RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm base-devel gcc lib32-gcc-libs clang \
    gtk3 brotli zstd \
    cmake git \
    && pacman -Scc --noconfirm \

# Set up a non-root user (recommended for CI security)
RUN groupadd -g 1001 github && \
    useradd -m -u 1001 -g github github

# Switch to the custom user
USER github

# Set the workdir
WORKDIR /home/github