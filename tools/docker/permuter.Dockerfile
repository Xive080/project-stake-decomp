# Imagen i386 para decomp-permuter + ee-gcc (los compiladores Linux son binarios i386).
FROM i386/debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
      python3 python3-pycparser python3-toml cpp git ca-certificates binutils-mipsel-linux-gnu binutils-mips-linux-gnu \
    && rm -rf /var/lib/apt/lists/*
RUN git clone --depth 1 https://github.com/simonlindholm/decomp-permuter.git /opt/permuter
WORKDIR /work
