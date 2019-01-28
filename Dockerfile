FROM ubuntu:18.04

# Install prerequisites
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libssl-dev \
    libmysqld-dev \
    autoconf \
    libtool \
    libffi-dev

ADD ./kbe kbengine/kbe

# Build engine source
RUN chmod -R 755 /kbengine/kbe
WORKDIR /kbengine/kbe/src
RUN make

# Build Python interpreter
WORKDIR /kbengine/kbe/src/lib/python
RUN ./configure
RUN make
RUN make install

RUN ulimit -c unlimited
ADD server_assets /kbengine/server_assets
RUN chmod 755 -R /kbengine/server_assets
RUN chmod -R 755 /kbengine/server_assets
WORKDIR /kbengine/server_assets