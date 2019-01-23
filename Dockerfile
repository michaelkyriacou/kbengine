FROM ubuntu:18.04

# Install prerequisites
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libssl-dev \
    mysql-server \
    libmysqld-dev \
    autoconf \
    libtool

ADD ./kbe/src kbengine/kbe/src

# Build engine source
WORKDIR /kbengine/kbe/src
RUN chmod -R 755 .
RUN make

# Build Python interpreter
WORKDIR /kbengine/src/lib/python
RUN ./configure
RUN make
RUN make install
