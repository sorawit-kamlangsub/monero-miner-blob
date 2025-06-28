FROM ubuntu:22.04

# Install build deps
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gcc \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Clone and build RandomX
RUN git clone --depth 1 https://github.com/tevador/RandomX.git /app/RandomX
RUN mkdir /app/RandomX/build && cd /app/RandomX/build && cmake .. && make -j

# Copy miner source, job.json, and Makefile
COPY main.c job.json Makefile /app/

# Build miner
RUN make

ENTRYPOINT ["./miner"]
