FROM archlinux:latest

LABEL org.opencontainers.image.description="An image used for building koality projects"

# Enable 32-bit repo
RUN echo -e '\n[multilib]\nInclude = /etc/pacman.d/mirrorlist' >> /etc/pacman.conf && \
    pacman -Syu --noconfirm

# Update and install build tools, compilers, and libraries
RUN pacman -S --noconfirm \
    cmake git tree zip base-devel gcc clang \
    \
    brotli     lib32-brotli \
    gcc-libs   lib32-gcc-libs \
    gtk3       lib32-gtk3 \
    libidn2    lib32-libidn2 \
    libnghttp2 lib32-libnghttp2 \
    libssh2    lib32-libssh2 \
    openssl    lib32-openssl \
    zlib       lib32-zlib \
    zstd       lib32-zstd

# Clear package cache
RUN pacman -Scc --noconfirm

# Setup github user
RUN useradd -u 1001 -m github
USER github
