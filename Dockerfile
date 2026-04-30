# ────────────────────────────────────────────
# Stage 1: builder
# ────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

# 빌드 도구 + boost 헤더 설치.
# DEBIAN_FRONTEND=noninteractive 는 apt 가 설치 중 prompt 띄우지 않게 한다.
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libboost-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 의존성 정의 파일을 먼저 복사하면 소스만 변경됐을 때 캐시 재활용 가능.
COPY CMakeLists.txt CMakePresets.json ./
COPY src ./src

RUN cmake --preset linux-release \
    && cmake --build --preset linux-release

# ────────────────────────────────────────────
# Stage 2: runtime
# ────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

# 런타임에는 빌드 도구 / 헤더 불필요.
# Boost.Asio 가 헤더 온리이므로 별도 라이브러리 설치도 필요 없다.
# 표준 C++ 런타임만 있으면 됨 (ubuntu base 에 이미 포함).

WORKDIR /app

# 빌드 산출물만 복사.
COPY --from=builder /app/build/game_server /app/game_server

EXPOSE 7777

ENTRYPOINT ["/app/game_server"]
CMD ["7777"]