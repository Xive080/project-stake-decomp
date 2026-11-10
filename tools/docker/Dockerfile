# Imagen base para el pipeline de byte-match de AirBlade.
# Trae lo que permuter/m2c necesitan en runtime SIN red:
#   - python3            -> m2c y permuter
#   - binutils-mips-...  -> mips-linux-gnu-as (ensamblar target.s) y -objdump (scoring)
#   - libc6:i386         -> para que wibo-i686 ejecute el mwccps2.exe de 32 bits
# El compilador (mwccps2) y wibo se montan por volumen, no van en la imagen.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        python3 python3-toml cpp binutils-mips-linux-gnu libc6:i386 ca-certificates && \
    rm -rf /var/lib/apt/lists/*
