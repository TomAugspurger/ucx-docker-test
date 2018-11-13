FROM continuumio/miniconda3
RUN apt-get update && apt-get install -y \
    build-essential \
    libnuma-dev \
    autoconf \
    libtool \
 && rm -rf /var/lib/apt/lists/*

ENV UCX_HOME /ucx
ENV UCX_PREFIX /ucx/install

COPY ucx /ucx

WORKDIR /ucx

RUN cd /ucx \
 && ./autogen.sh \
 && ./configure --prefix=$UCX_PREFIX \
 && make -j 4 install