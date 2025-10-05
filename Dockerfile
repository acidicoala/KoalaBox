FROM archlinux:latest

LABEL org.opencontainers.image.description="An image used for building koality projects"

# Enable 32-bit repo
RUN echo -e '\n[multilib]\nInclude = /etc/pacman.d/mirrorlist' >> /etc/pacman.conf && \
    pacman -Syu --noconfirm

# Update and install build tools, compilers, and libraries
RUN pacman -S --noconfirm \
    cmake git \
    base-devel gcc lib32-gcc-libs clang \
    gtk3 brotli zstd libssh2 lib32-libssh2

# Clear package cache
RUN pacman -Scc --noconfirm
