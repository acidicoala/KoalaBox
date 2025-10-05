FROM archlinux:latest

# Update and install build tools and compilers
RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm base-devel gcc lib32-gcc-libs clang \
    gtk3 brotli zstd \
    cmake git \
    && pacman -Scc --noconfirm
