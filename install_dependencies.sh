#!/bin/bash

echo "CPU 서버 의존성 설치 중..."

# macOS에서 의존성 설치
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "macOS 환경에서 의존성 설치"
    
    # Homebrew 확인
    if ! command -v brew &> /dev/null; then
        echo "Homebrew가 설치되지 않았습니다. 설치를 진행합니다."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # libwebsockets 설치
    echo "libwebsockets 설치 중..."
    brew install libwebsockets
    
    # json-c 설치
    echo "json-c 설치 중..."
    brew install json-c
    
    # pkg-config 설치
    echo "pkg-config 설치 중..."
    brew install pkg-config

# Ubuntu/Debian에서 의존성 설치
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Linux 환경에서 의존성 설치"
    
    # 패키지 목록 업데이트
    sudo apt-get update
    
    # libwebsockets 설치
    echo "libwebsockets 설치 중..."
    sudo apt-get install -y libwebsockets-dev
    
    # json-c 설치
    echo "json-c 설치 중..."
    sudo apt-get install -y libjson-c-dev
    
    # pkg-config 설치
    echo "pkg-config 설치 중..."
    sudo apt-get install -y pkg-config
    
    # build-essential 설치
    sudo apt-get install -y build-essential cmake

else
    echo "지원되지 않는 운영체제입니다: $OSTYPE"
    exit 1
fi

echo "의존성 설치가 완료되었습니다!"
echo ""
echo "빌드하려면 다음 명령어를 실행하세요:"
echo "mkdir -p build && cd build"
echo "cmake .."
echo "make" 