#include "include/websocket_server.h"
#include "include/cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// 신호 처리기
static void signal_handler(int sig) {
    printf("\n서버를 종료합니다...\n");
    ws_server_cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    int port = WS_PORT;
    
    // 명령행 인자로 포트 지정 가능
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "잘못된 포트 번호: %s\n", argv[1]);
            return 1;
        }
    }
    
    // 신호 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("CPU WebSocket 서버 시작\n");
    printf("포트: %d\n", port);
    printf("Ctrl+C로 종료\n\n");
    
    // WebSocket 서버 초기화
    if (ws_server_init(port) != 0) {
        fprintf(stderr, "서버 초기화 실패\n");
        return 1;
    }
    
    // 서버 실행
    int result = ws_server_run();
    
    // 정리
    ws_server_cleanup();
    
    return result;
} 