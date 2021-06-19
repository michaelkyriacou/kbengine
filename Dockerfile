FROM ubuntu:21.04

LABEL maintainer="michael.kyriacou@alkstudios.com"

ENV KBE_DEMO_VERSION 2.5.1
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update -y
RUN apt install -y wget unzip gcc g++ make libtool autoconf libssl-dev default-libmysqlclient-dev

COPY kbe/ /kbengine/kbe/

# WORKDIR /kbengine/kbe/src

# RUN make

WORKDIR /kbengine/

# # Demo
RUN wget https://github.com/kbengine/kbengine_demos_assets/archive/refs/tags/v$KBE_DEMO_VERSION.zip
RUN unzip v$KBE_DEMO_VERSION.zip && \
	rm v$KBE_DEMO_VERSION.zip && \
	mv kbengine_demos_assets-$KBE_DEMO_VERSION kbengine_demos_assets && \
	chmod -R 755 /kbengine/kbengine_demos_assets

# RUN apt install -y libffi-dev
# RUN apt install -y libncurses5-dev

# WORKDIR /kbengine/kbe/src/lib/python
# RUN ./configure
# RUN make
# RUN make install

# 替换python版本（V1.3.13 编译有问题）
ENV pythonVersion 3.7.9
# 上传python
ADD Python-$pythonVersion.zip .
# 重新编译 python
RUN rm -rf $kbeDir/kbengine/kbe/src/lib/python
RUN unzip Python-$pythonVersion.zip
RUN mv Python-$pythonVersion /kbengine/kbe/src/lib/python

WORKDIR /kbengine/kbe/src/lib/python

RUN ./configure --enable-optimizations
RUN make
RUN make install


# Set execuate enviroment
WORKDIR /kbengine/kbengine_demos_assets

# Run server
CMD ["./start_server.sh"]

EXPOSE 80
