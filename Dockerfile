# CPU Backend Dockerfile
FROM ubuntu:22.04

# 시간대 설정 (대화형 입력 방지)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y tzdata

# 필요한 패키지 설치
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    cmake \
    libwebsockets-dev \
    libjson-c-dev \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 소스 코드 복사
COPY . .

# CMake 빌드 디렉토리 생성
RUN mkdir -p cmake-build-docker

# CMake로 빌드
WORKDIR /app/cmake-build-docker
RUN cmake .. && make

# 실행 파일이 있는 위치로 이동
WORKDIR /app

# 포트 노출
EXPOSE 8080

# 실행 명령
CMD ["./cmake-build-docker/cpu"] 