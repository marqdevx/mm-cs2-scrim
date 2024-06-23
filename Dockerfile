FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

ENV HL2SDKCS2='/dockerVolume/mm-cs2-scrim/sdk'
ENV MMSOURCE_DEV='/dockerVolume/metamod-source'
ENV CC=clang
ENV CXX=clang++

RUN echo "Installing ambuild" && \
    apt-get update && \
    apt-get install python3-setuptools -y && \
    apt-get install clang -y && \
    apt-get install python3 -y && \
    apt-get install gcc -y

        #apt update && apt install -y clang-16
RUN apt-get install git -y

RUN git clone https://github.com/alliedmodders/ambuild.git

RUN cd ambuild && \
    python setup.py install
    