FROM continuumio/miniconda3
# https://hpc.github.io/charliecloud/tutorial.html

RUN apt-get update && apt-get install -y \
    autoconf \
    build-essential \
    libnuma-dev \
    libtool \
    libdb5.3-dev \
    libhwloc-dev \
    libnl-3-200 \
    libnl-route-3-200 \
    libnuma1 \
    libpmi2-0-dev \
 && rm -rf /var/lib/apt/lists/*

ENV UCX_HOME /ucx
ENV UCX_PREFIX /ucx/install
ENV MPI_URL https://www.open-mpi.org/software/ompi/v2.1/downloads
ENV MPI_VERSION 2.1.5

COPY ucx /ucx

WORKDIR /ucx

RUN cd /ucx \
 && ./autogen.sh \
 && ./configure --prefix=$UCX_PREFIX \
 && make -j 4 install

RUN wget -nv ${MPI_URL}/openmpi-${MPI_VERSION}.tar.gz
RUN tar xf openmpi-${MPI_VERSION}.tar.gz
RUN    cd openmpi-${MPI_VERSION} \
    && CFLAGS=-O3 \
       CXXFLAGS=-O3 \
       ./configure --prefix=/usr/local \
                   --sysconfdir=/mnt/0 \
                   --with-pmi \
                   --with-pmi-libdir=/usr/lib/x86_64-linux-gnu \
                   --with-pmix \
                   --with-ucx=$UCX_PREFIX \
                   --enable-mca-no-build=btl-openib \
                   --with-slurm=no \
    && make -j$(getconf _NPROCESSORS_ONLN) install
RUN ldconfig
RUN rm -Rf openmpi-${MPI_VERSION}*
# OpenMPI expects this program to exist, even if it's not used. Default is
# "ssh : rsh", but that's not installed.
RUN echo 'plm_rsh_agent = false' >> /mnt/0/openmpi-mca-params.conf
