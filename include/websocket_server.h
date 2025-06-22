#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>
#include <json-c/json.h>
#include <stdint.h>
#include "cpu.h"

#define WS_PORT 8080
#define MAX_PAYLOAD_SIZE 4096
#define MAX_CLIENTS 10

// 메시지 타입 정의
typedef enum {
    MSG_TYPE_ASSEMBLY,      // 어셈블리 코드 수신
    MSG_TYPE_STATE,         // CPU 상태 전송
    MSG_TYPE_MEMORY,        // 메모리 상태 전송
    MSG_TYPE_REGISTER,      // 레지스터 상태 전송
    MSG_TYPE_EXECUTION,     // 명령어 실행 단계
    MSG_TYPE_ACK,           // 확인 응답
    MSG_TYPE_ERROR,         // 오류 메시지
    MSG_TYPE_PING,          // 핑
    MSG_TYPE_PONG           // 퐁
} message_type_t;

// WebSocket 클라이언트 세션 정보
typedef struct {
    struct lws *wsi;
    char ip_addr[32];
    int session_id;
    int is_connected;
} ws_client_session_t;

// CPU 실행 상태 정보
typedef struct {
    int is_running;
    int step_mode;
    int current_instruction;
    char last_assembly[256];
    uint8_t decoded_bytes[256];
    int decoded_length;
} cpu_execution_state_t;

// WebSocket 서버 컨텍스트
typedef struct {
    struct lws_context *context;
    ws_client_session_t clients[MAX_CLIENTS];
    int client_count;
    cpu_execution_state_t cpu_state;
    pthread_mutex_t mutex;
} ws_server_context_t;

// WebSocket 서버 함수들
int ws_server_init(int port);
void ws_server_cleanup(void);
int ws_server_run(void);

// 메시지 전송 함수들
void ws_send_cpu_state(void);
void ws_send_memory_state(void);
void ws_send_register_state(void);
void ws_send_execution_step(const char* instruction, const uint8_t* bytes, int byte_count);
void ws_send_error(const char* error_msg);
void ws_send_ack(const char* message);

// CPU 제어 함수들
int ws_handle_assembly_code(const char* assembly_code);
int ws_handle_program_load(const char* program_code);
int ws_handle_step_execution(void);
int ws_handle_cpu_reset(void);
int ws_handle_run_all(void);
void ws_execute_instruction_step(void);
void ws_reset_cpu(void);

// JSON 메시지 처리 함수들
json_object* create_state_message(void);
json_object* create_memory_message(void);
json_object* create_register_message(void);
json_object* create_execution_message(const char* instruction, const uint8_t* bytes, int byte_count);
json_object* create_error_message(const char* error);
json_object* create_ack_message(const char* message);

// 어셈블리 디코딩 함수
int decode_assembly_to_bytes(const char* assembly, uint8_t* output_bytes, int max_length);
int decode_bytes_to_assembly(const uint8_t* bytes, int byte_count, char* output_assembly, int max_length);

#endif // WEBSOCKET_SERVER_H 