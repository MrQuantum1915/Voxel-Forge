# FROM ubuntu:22.04

# # Install build tools and required dependencies for COLMAP and Qt5
# RUN apt-get update && apt-get install -y \
#     git build-essential cmake qtbase5-dev qt5-qmake qtbase5-dev-tools \
#     libboost-all-dev libeigen3-dev libfreeimage-dev libgoogle-glog-dev libgflags-dev \
#     libopencv-dev libcgal-dev libcgal-qt5-dev libusb-1.0-0-dev libsuitesparse-dev \
#     && apt-get clean && rm -rf /var/lib/apt/lists/*

# # Clone and build COLMAP from source
# RUN git clone https://github.com/colmap/colmap.git /colmap && \
#     mkdir /colmap/build && cd /colmap/build && \
#     cmake .. && \
#     make -j$(nproc) && \
#     make install

# WORKDIR /src

# COPY . /src

# RUN mkdir build
# WORKDIR /build

# RUN cmake ..
# RUN cmake --build .

# CMD ["./Voxel-Forge"]
FROM rabits/qt:5.14-desktop

USER root

RUN apt-get update && apt-get install -y \
    git build-essential cmake libboost-all-dev libeigen3-dev libfreeimage-dev libgoogle-glog-dev libgflags-dev \
    libopencv-dev libcgal-dev libcgal-qt5-dev libusb-1.0-0-dev libsuitesparse-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/colmap/colmap.git /colmap && \
    mkdir /colmap/build && cd /colmap/build && \
    cmake .. && make -j$(nproc) && make install

WORKDIR /src
COPY . /src

RUN mkdir /build && cd /build && \
    cmake /src && cmake --build .

CMD ["/build/Voxel-Forge"]
