FROM archlinux:latest

LABEL org.opencontainers.image.description="An image used for building koality projects"

# Enable 32-bit repo
RUN echo -e '\n[multilib]\nInclude = /etc/pacman.d/mirrorlist' >> /etc/pacman.conf && \
    pacman -Syu --noconfirm

# Update and install build tools, compilers, and libraries
RUN pacman -S --noconfirm \
    cmake git tree base-devel \
    clang \
    gcc        lib32-gcc-libs \
    gtk3       lib32-gtk3 \
    brotli     lib32-brotli \
    zstd       lib32-zstd \
    libnghttp2 lib32-libnghttp2 \
    libssh2    lib32-libssh2 \
    libidn2    lib32-libidn2

# Clear package cache
RUN pacman -Scc --noconfirm
