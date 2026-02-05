# ============================================================================
# Stage 0: Common base, defined here like a constant for multiple subsequent stages
# ============================================================================
FROM ghcr.io/linuxserver/baseimage-ubuntu:noble AS base

# ============================================================================
# Stage 1: build-base for all build steps
# ============================================================================
FROM base AS plugin-base

ARG DEBIAN_FRONTEND="noninteractive"

# Install common build packages used by all plugin builders
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    7zip \
    build-essential \
    cmake \
    extra-cmake-modules \
    g++ \
    gcc \
    git \
    make \
    qt6-base-dev \
    qt6-base-private-dev \
    unzip \
    wget \
    pkg-config && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# ============================================================================
# Stage 2: kimageformats builder
# ============================================================================
FROM plugin-base AS kimageformats-builder

# Install additional dependencies for kimageformats
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libavif-dev \
    libjxl-dev \
    libarchive-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Build ECM 6.0.0 for KF6 (compatible with Qt 6.4)
WORKDIR /tmp/ecm
RUN git clone --depth 1 --branch v6.0.0 https://invent.kde.org/frameworks/extra-cmake-modules.git .
RUN mkdir build
WORKDIR /tmp/ecm/build
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF
RUN make -j$(nproc)
RUN make install

# Build kimageformats 6.0.0 from KDE source (patched for Qt 6.4)
WORKDIR /tmp/kimageformats
RUN git clone --depth 1 --branch v6.0.0 https://invent.kde.org/frameworks/kimageformats.git .
# Patch to work with Qt 6.4 (Noble has 6.4.2, kimageformats wants 6.5+)
RUN sed -i 's/6\.5\.0/6.4.0/g' CMakeLists.txt
RUN mkdir build
WORKDIR /tmp/kimageformats/build
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF \
    -DQT_MAJOR_VERSION=6
RUN make -j$(nproc)
RUN make install DESTDIR=/output

# ============================================================================
# Stage 3: 7zip builder
# ============================================================================
FROM plugin-base AS sevenzip-builder

# Download 7z source
RUN mkdir -p /src/7zip
WORKDIR /src/7zip
RUN wget "https://github.com/YACReader/yacreader-7z-deps/blob/main/7z2301-src.7z?raw=true" -O 7z2301-src.7z

# Extract 7z source
RUN 7z x 7z2301-src.7z -o/src/7zip/lib7zip && \
    rm 7z2301-src.7z

# Build 7z.so with RAR support
WORKDIR /src/7zip/lib7zip/CPP/7zip/Bundles/Format7zF
RUN mkdir -p _o
RUN make -f makefile.gcc -j$(nproc)
RUN mkdir -p /app/lib/7zip && \
    cp ./_o/7z.so /app/lib/7zip

# ============================================================================
# Stage 4: YACReader builder
# ============================================================================
FROM plugin-base AS yacreader-builder

ARG YACR_VERSION="develop"
ENV APPNAME="YACReaderLibraryServer"
ENV HOME="/config"
LABEL maintainer="luisangelsm"

# Install YACReader-specific build packages (common packages inherited from plugin-base)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    desktop-file-utils \
    qt6-tools-dev \
    qt6-base-dev-tools \
    qmake6 \
    qmake6-bin \
    qt6-multimedia-dev \
    qt6-tools-dev-tools \
    libgl-dev \
    qt6-l10n-tools \
    libqt6opengl6-dev \
    libunarr-dev \
    qt6-declarative-dev \
    libqt6svg6-dev \
    libqt6core5compat6-dev \
    libbz2-dev \
    libglu1-mesa-dev \
    liblzma-dev \
    libqt6gui6 \
    libqt6multimedia6 \
    libqt6network6 \
    libqt6qml6 \
    libqt6quickcontrols2-6 \
    qt6-image-formats-plugins \
    libqt6sql6 \
    libqt6sql6-sqlite \
    sqlite3 \
    libsqlite3-dev \
    7zip-rar \
    libpoppler-qt6-dev \
    zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    ldconfig

# Clone YACReader repository
RUN git clone https://github.com/YACReader/yacreader.git /src/git && \
    cd /src/git && \
    git checkout $YACR_VERSION

# Copy pre-built 7z library and headers
COPY --from=sevenzip-builder /src/7zip/lib7zip /src/git/compressed_archive/lib7zip
COPY --from=sevenzip-builder /app/lib/7zip /app/lib/7zip

# Copy kimageformats Qt6 plugins
COPY --from=kimageformats-builder /output/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/ \
    /usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/

# Build YACReaderLibraryServer
RUN cd /src/git/YACReaderLibraryServer && \
    qmake6 PREFIX=/app CONFIG+="7zip server_standalone" YACReaderLibraryServer.pro && \
    qmake6 -v && \
    make -j$(nproc) && \
    make install

# ============================================================================
# Stage 5: Final runtime image (minimal, no build assets)
# ============================================================================
FROM base AS runtime

ENV APPNAME="YACReaderLibraryServer"
ENV HOME="/config"
ENV LC_ALL="en_US.UTF-8"
ENV PATH="/app/bin:${PATH}"
LABEL maintainer="luisangelsm"

# Install only runtime dependencies (no build tools)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libqt6core5compat6 \
    libpoppler-qt6-3t64 \
    qt6-image-formats-plugins \
    libavif16 \
    libjxl0.7 \
    libqt6network6t64 \
    libqt6sql6-sqlite && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Copy YACReaderLibraryServer application from builder
COPY --from=yacreader-builder /app /app

# Copy kimageformats Qt6 plugins (must be done before ldconfig)
COPY --from=kimageformats-builder /output/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/*.so \
    /usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/

# Refresh dynamic linker cache
RUN ldconfig

# Copy linuxserver.io configuration
COPY docker/root.tar.gz /
RUN tar -xvpzf /root.tar.gz -C / && rm /root.tar.gz

# Expose server port and define volumes
EXPOSE 8080
VOLUME ["/config", "/comics"]
