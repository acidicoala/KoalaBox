FROM archlinux:latest

LABEL org.opencontainers.image.description="An image used for building koality projects"

# Update and install build tools and compilers
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm base-devel gcc lib32-gcc-libs clang \
    gtk3 brotli zstd \
    cmake git && \
    pacman -Scc --noconfirm

# Set up a non-root user (required to avoid dubious ownership git error)
ARG UNAME=github
ARG UID=1001
ARG GID=1001
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
USER $UNAME

# Set the workdir
WORKDIR /home/$UNAME