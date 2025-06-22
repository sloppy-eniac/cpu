#include "include/websocket_server.h"
#include "include/cpu.h"
#include "include/cache.h"
#include <libwebsockets.h>
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 전역 서버 컨텍스트
static ws_server_context_t server_ctx;
static int server_running = 0;

// 프로토콜 콜백 함수
static int callback_cpu_protocol(struct lws *wsi, enum lws_callback_reasons reason,
                                void *user, void *in, size_t len);

// 프로토콜 정의
static struct lws_protocols protocols[] = {
    {
        "cpu-protocol",
        callback_cpu_protocol,
        0,
        MAX_PAYLOAD_SIZE,
    },
    { NULL, NULL, 0, 0 } // 종료자
};

// WebSocket 서버 초기화
int ws_server_init(int port) {
    struct lws_context_creation_info info;
    
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    
    // 서버 컨텍스트 초기화
    memset(&server_ctx, 0, sizeof(server_ctx));
    pthread_mutex_init(&server_ctx.mutex, NULL);
    
    // CPU 초기화
    cpu_init();
    
    server_ctx.context = lws_create_context(&info);
    if (!server_ctx.context) {
        fprintf(stderr, "WebSocket 컨텍스트 생성 실패\n");
        return -1;
    }
    
    printf("WebSocket 서버가 포트 %d에서 시작되었습니다\n", port);
    server_running = 1;
    return 0;
}

// WebSocket 서버 정리
void ws_server_cleanup(void) {
    server_running = 0;
    if (server_ctx.context) {
        lws_context_destroy(server_ctx.context);
        server_ctx.context = NULL;
    }
    pthread_mutex_destroy(&server_ctx.mutex);
}

// 클라이언트 추가
static int add_client(struct lws *wsi) {
    pthread_mutex_lock(&server_ctx.mutex);
    
    if (server_ctx.client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&server_ctx.mutex);
        return -1;
    }
    
    int client_id = server_ctx.client_count;
    server_ctx.clients[client_id].wsi = wsi;
    server_ctx.clients[client_id].session_id = client_id;
    server_ctx.clients[client_id].is_connected = 1;
    
    // 클라이언트 IP 주소 얻기
    char client_name[128], client_ip[128];
    lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), client_name, sizeof(client_name), client_ip, sizeof(client_ip));
    strncpy(server_ctx.clients[client_id].ip_addr, client_ip, sizeof(server_ctx.clients[client_id].ip_addr) - 1);
    
    server_ctx.client_count++;
    
    printf("클라이언트 연결됨: %s (세션 ID: %d)\n", client_ip, client_id);
    
    pthread_mutex_unlock(&server_ctx.mutex);
    return client_id;
}

// 클라이언트 제거
static void remove_client(struct lws *wsi) {
    pthread_mutex_lock(&server_ctx.mutex);
    
    for (int i = 0; i < server_ctx.client_count; i++) {
        if (server_ctx.clients[i].wsi == wsi) {
            printf("클라이언트 연결 해제: %s (세션 ID: %d)\n", 
                   server_ctx.clients[i].ip_addr, server_ctx.clients[i].session_id);
            
            // 배열에서 제거 (마지막 요소를 현재 위치로 이동)
            if (i < server_ctx.client_count - 1) {
                server_ctx.clients[i] = server_ctx.clients[server_ctx.client_count - 1];
            }
            server_ctx.client_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&server_ctx.mutex);
}

// 모든 클라이언트에게 메시지 전송
static void broadcast_message(const char* message) {
    pthread_mutex_lock(&server_ctx.mutex);
    
    for (int i = 0; i < server_ctx.client_count; i++) {
        if (server_ctx.clients[i].is_connected) {
            size_t msg_len = strlen(message);
            unsigned char *buf = malloc(LWS_PRE + msg_len);
            
            if (buf) {
                memcpy(&buf[LWS_PRE], message, msg_len);
                lws_write(server_ctx.clients[i].wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                free(buf);
            }
        }
    }
    
    pthread_mutex_unlock(&server_ctx.mutex);
}

// 어셈블리 코드를 바이트로 디코딩
int decode_assembly_to_bytes(const char* assembly, uint8_t* output_bytes, int max_length) {
    // 간단한 어셈블리 파서 구현
    // 예: "ADD 1, 5" -> 0x01, 0x05
    // 예: "SUB 3, 7" -> 0x23, 0x07
    // 예: "ADD 10" -> 0x0A, 0x00 (단일 피연산자)
    
    if (!assembly || !output_bytes || max_length < 2) {
        return 0;
    }
    
    char instruction[32];
    int reg1, reg2 = 0;
    
    // 두 개의 피연산자를 시도
    int parsed = sscanf(assembly, "%s %d, %d", instruction, &reg1, &reg2);
    
    // 한 개의 피연산자만 있는 경우
    if (parsed == 2) {
        reg2 = 0; // 두 번째 피연산자는 0으로 설정
    } else if (parsed != 3) {
        return 0; // 파싱 실패
    }
    
    uint8_t opcode = 0;
    if (strcmp(instruction, "ADD") == 0) opcode = 0;
    else if (strcmp(instruction, "SUB") == 0) opcode = 1;
    else if (strcmp(instruction, "MUL") == 0) opcode = 2;
    else if (strcmp(instruction, "DIV") == 0) opcode = 3;
    else if (strcmp(instruction, "MOV") == 0) opcode = 4;
    else return 0;
    
    // 16비트 명령어 생성: 4비트 opcode + 6비트 reg1 + 6비트 reg2
    uint16_t instruction_word = (opcode << 12) | ((reg1 & 0x3F) << 6) | (reg2 & 0x3F);
    
    output_bytes[0] = (instruction_word >> 8) & 0xFF;
    output_bytes[1] = instruction_word & 0xFF;
    
    return 2;
}

// 바이트를 어셈블리 코드로 변환 (역변환)
int decode_bytes_to_assembly(const uint8_t* bytes, int byte_count, char* output_assembly, int max_length) {
    if (!bytes || byte_count < 2 || !output_assembly || max_length < 32) {
        return 0;
    }
    
    // 16비트 명령어 재구성
    uint16_t instruction_word = (bytes[0] << 8) | bytes[1];
    
    // 필드 추출
    uint8_t opcode = (instruction_word >> 12) & 0xF;
    uint8_t reg1 = (instruction_word >> 6) & 0x3F;
    uint8_t reg2 = instruction_word & 0x3F;
    
    // opcode에 따른 명령어 문자열 생성
    const char* op_name;
    switch (opcode) {
        case 0: op_name = "ADD"; break;
        case 1: op_name = "SUB"; break;
        case 2: op_name = "MUL"; break;
        case 3: op_name = "DIV"; break;
        case 4: op_name = "MOV"; break;
        default: return 0;
    }
    
    snprintf(output_assembly, max_length, "%s %d, %d", op_name, reg1, reg2);
    return 1;
}

// JSON 메시지 생성 함수들
json_object* create_state_message(void) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("state");
    json_object *payload = json_object_new_object();
    
    CPU_Registers *regs = get_cpu_registers();
    
    json_object *pc = json_object_new_int(regs->pc);
    json_object *reg1 = json_object_new_int(regs->register1);
    json_object *reg2 = json_object_new_int(regs->register2);
    json_object *reg3 = json_object_new_int(regs->register3);
    
    json_object_object_add(payload, "pc", pc);
    json_object_object_add(payload, "register1", reg1);
    json_object_object_add(payload, "register2", reg2);
    json_object_object_add(payload, "register3", reg3);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

json_object* create_memory_message(void) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("memory");
    json_object *payload = json_object_new_object();
    
    Memory *memory = get_cpu_memory();
    json_object *memory_array = json_object_new_array();
    
    for (int i = 0; i < MEMORY_SIZE && i < 64; i++) { // 처음 64바이트만 전송
        json_object *byte_val = json_object_new_int(memory->data[i]);
        json_object_array_add(memory_array, byte_val);
    }
    
    json_object_object_add(payload, "data", memory_array);
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

json_object* create_cache_message(void) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("cache");
    json_object *payload = json_object_new_object();
    
    Memory *memory = get_cpu_memory();
    Cache *cache = &memory->cache;
    
    json_object *cache_lines = json_object_new_array();
    
    // 처음 16개 캐시 라인만 전송 (화면에 보여줄 수 있는 적당한 양)
    for (int i = 0; i < 16 && i < 64; i++) {
        CacheLine *line = &cache->lines[i];
        
        json_object *line_obj = json_object_new_object();
        json_object *index = json_object_new_int(i);
        json_object *tag = json_object_new_int(line->tag);
        json_object *valid = json_object_new_boolean(line->valid);
        json_object *dirty = json_object_new_boolean(line->dirty);
        
        // 캐시 라인의 4바이트 데이터
        json_object *data_array = json_object_new_array();
        for (int j = 0; j < 4; j++) {
            json_object *byte_val = json_object_new_int(line->block[j]);
            json_object_array_add(data_array, byte_val);
        }
        
        json_object_object_add(line_obj, "index", index);
        json_object_object_add(line_obj, "tag", tag);
        json_object_object_add(line_obj, "valid", valid);
        json_object_object_add(line_obj, "dirty", dirty);
        json_object_object_add(line_obj, "data", data_array);
        
        json_object_array_add(cache_lines, line_obj);
    }
    
    json_object_object_add(payload, "lines", cache_lines);
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

json_object* create_execution_message(const char* instruction, const uint8_t* bytes, int byte_count) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("execution");
    json_object *payload = json_object_new_object();
    
    json_object *inst_str = json_object_new_string(instruction);
    json_object *bytes_array = json_object_new_array();
    
    for (int i = 0; i < byte_count; i++) {
        json_object *byte_val = json_object_new_int(bytes[i]);
        json_object_array_add(bytes_array, byte_val);
    }
    
    json_object_object_add(payload, "instruction", inst_str);
    json_object_object_add(payload, "bytes", bytes_array);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

json_object* create_ack_message(const char* message) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("ack");
    json_object *payload = json_object_new_string(message);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

json_object* create_error_message(const char* error) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("error");
    json_object *payload = json_object_new_string(error);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

// 메시지 전송 함수들
void ws_send_cpu_state(void) {
    json_object *msg = create_state_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

void ws_send_memory_state(void) {
    json_object *msg = create_memory_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

void ws_send_cache_state(void) {
    json_object *msg = create_cache_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

void ws_send_execution_step(const char* instruction, const uint8_t* bytes, int byte_count) {
    json_object *msg = create_execution_message(instruction, bytes, byte_count);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

void ws_send_ack(const char* message) {
    json_object *msg = create_ack_message(message);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

void ws_send_error(const char* error_msg) {
    json_object *msg = create_error_message(error_msg);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

// 어셈블리 코드 처리
int ws_handle_assembly_code(const char* assembly_code) {
    uint8_t bytes[256];
    int byte_count = decode_assembly_to_bytes(assembly_code, bytes, sizeof(bytes));
    
    if (byte_count <= 0) {
        ws_send_error("어셈블리 디코딩 실패");
        return -1;
    }
    
    // CPU에 프로그램 로드
    cpu_load_program(bytes, byte_count);
    
    // 실행 단계 전송
    ws_send_execution_step(assembly_code, bytes, byte_count);
    
    // CPU 한 단계 실행
    cpu_step();
    
    // 상태 전송
    ws_send_cpu_state();
    ws_send_memory_state();
    ws_send_cache_state();
    
    return 0;
}

// 프로그램 로드 처리 (여러 줄 어셈블리)
int ws_handle_program_load(const char* program_code) {
    if (!program_code) {
        ws_send_error("프로그램 코드가 없습니다");
        return -1;
    }
    
    printf("프로그램 로드 요청: %s\n", program_code);
    
    // 프로그램을 한 줄씩 나누어 처리
    char *program_copy = strdup(program_code);
    char *line = strtok(program_copy, "\n");
    uint8_t all_bytes[1024];
    int total_byte_count = 0;
    
    while (line != NULL && total_byte_count < sizeof(all_bytes) - 2) {
        // 공백 제거
        while (*line == ' ' || *line == '\t') line++;
        if (strlen(line) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }
        
        // 각 줄을 바이트로 변환
        uint8_t line_bytes[16];
        int line_byte_count = decode_assembly_to_bytes(line, line_bytes, sizeof(line_bytes));
        
        if (line_byte_count > 0) {
            memcpy(all_bytes + total_byte_count, line_bytes, line_byte_count);
            total_byte_count += line_byte_count;
            printf("라인 처리: %s -> %d 바이트\n", line, line_byte_count);
        } else {
            printf("라인 디코딩 실패: %s\n", line);
        }
        
        line = strtok(NULL, "\n");
    }
    
    if (total_byte_count > 0) {
        // CPU 초기화 (모든 레지스터와 메모리)
        cpu_reset();
        
        // CPU에 전체 프로그램 로드 (실행하지 않고 메모리에만 로드)
        cpu_load_program(all_bytes, total_byte_count);
        
        // 성공 메시지 전송
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), "프로그램 로드 완료: %d 바이트 (실행 대기 중)", total_byte_count);
        ws_send_ack(success_msg);
        
        // 실행 단계 전송
        ws_send_execution_step("프로그램 로드됨 - 단계 실행 준비", all_bytes, total_byte_count);
        
        // 상태 전송
        ws_send_cpu_state();
        ws_send_memory_state();
        ws_send_cache_state();
        
        printf("프로그램 로드 성공: %d 바이트 (단계 실행 준비)\n", total_byte_count);
    } else {
        ws_send_error("프로그램에서 유효한 명령어를 찾을 수 없습니다");
        free(program_copy);
        return -1;
    }
    
    free(program_copy);
    return 0;
}

// 단계별 실행 처리
int ws_handle_step_execution(void) {
    printf("단계 실행 요청\n");
    
    // 실행 전 PC 저장
    CPU_Registers *regs = get_cpu_registers();
    int prev_pc = regs->pc;
    
    // 현재 PC 위치의 명령어 가져오기 (실행 전)
    Memory *memory = get_cpu_memory();
    char current_instruction[64] = "알 수 없는 명령어";
    
    if (prev_pc < MEMORY_SIZE - 1) {
        uint8_t instruction_bytes[2];
        instruction_bytes[0] = memory->data[prev_pc];
        instruction_bytes[1] = memory->data[prev_pc + 1];
        
        // 바이트를 어셈블리로 변환
        if (decode_bytes_to_assembly(instruction_bytes, 2, current_instruction, sizeof(current_instruction))) {
            printf("실행할 명령어: %s (PC: %d)\n", current_instruction, prev_pc);
        }
    }
    
    // CPU 한 단계 실행
    cpu_step();
    
    // 실행 후 PC 확인
    regs = get_cpu_registers();
    
    // 실행된 명령어 정보 전송
    char step_msg[256];
    snprintf(step_msg, sizeof(step_msg), "실행: %s | PC: %d -> %d", current_instruction, prev_pc, regs->pc);
    
    // 실행 단계 전송 (실행된 명령어와 바이트 정보 포함)
    uint8_t executed_bytes[2] = {0, 0};
    if (prev_pc < MEMORY_SIZE - 1) {
        executed_bytes[0] = memory->data[prev_pc];
        executed_bytes[1] = memory->data[prev_pc + 1];
    }
    ws_send_execution_step(step_msg, executed_bytes, 2);
    
    // 상태 전송
    ws_send_cpu_state();
    ws_send_memory_state();
    ws_send_cache_state();
    
    ws_send_ack("단계 실행 완료");
    printf("단계 실행 성공: %s | PC %d -> %d\n", current_instruction, prev_pc, regs->pc);
    
    return 0;
}

// CPU 리셋 처리
int ws_handle_cpu_reset(void) {
    printf("CPU 리셋 요청\n");
    
    // CPU 초기화
    cpu_reset();
    
    // 상태 전송
    ws_send_cpu_state();
    ws_send_memory_state();
    ws_send_cache_state();
    
    // 실행 단계 전송
    ws_send_execution_step("CPU 리셋 완료", NULL, 0);
    
    ws_send_ack("CPU 리셋 완료");
    printf("CPU 리셋 성공\n");
    
    return 0;
}

// 전체 프로그램 일괄 실행 처리
int ws_handle_run_all(void) {
    printf("전체 프로그램 실행 요청\n");
    
    CPU_Registers *regs = get_cpu_registers();
    Memory *memory = get_cpu_memory();
    int initial_pc = regs->pc;
    int step_count = 0;
    int max_steps = 16; // 최대 16단계까지 실행 (무한 루프 방지)
    
    // 실행 시작 메시지 전송
    ws_send_execution_step("전체 프로그램 실행 시작", NULL, 0);
    
    // 프로그램이 끝날 때까지 단계별 실행
    while (step_count < max_steps && regs->pc < MEMORY_SIZE - 1) {
        // 현재 명령어 확인
        if (memory->data[regs->pc] == 0 && memory->data[regs->pc + 1] == 0) {
            printf("빈 명령어 도달 - 실행 종료 (PC: %d)\n", regs->pc);
            break;
        }
        
        // 실행 전 PC와 명령어 저장
        int prev_pc = regs->pc;
        char current_instruction[64] = "알 수 없는 명령어";
        uint8_t instruction_bytes[2];
        instruction_bytes[0] = memory->data[prev_pc];
        instruction_bytes[1] = memory->data[prev_pc + 1];
        
        // 바이트를 어셈블리로 변환
        decode_bytes_to_assembly(instruction_bytes, 2, current_instruction, sizeof(current_instruction));
        
        printf("실행 중: %s (PC: %d, 단계: %d)\n", current_instruction, prev_pc, step_count + 1);
        
        // CPU 한 단계 실행
        cpu_step();
        
        // 실행 결과 메시지 생성
        char step_msg[256];
        snprintf(step_msg, sizeof(step_msg), "단계 %d: %s | PC: %d -> %d", 
                step_count + 1, current_instruction, prev_pc, regs->pc);
        
        // 실행 단계 정보 전송
        ws_send_execution_step(step_msg, instruction_bytes, 2);
        
        // 잠시 대기 (시각적 효과를 위해)
        usleep(200000); // 200ms 대기
        
        step_count++;
    }
    
    // 최종 상태 전송
    ws_send_cpu_state();
    ws_send_memory_state();
    ws_send_cache_state();
    
    // 완료 메시지 전송
    char completion_msg[256];
    if (step_count >= max_steps) {
        snprintf(completion_msg, sizeof(completion_msg), 
                "전체 실행 완료 (최대 단계 도달): %d단계 실행", step_count);
    } else {
        snprintf(completion_msg, sizeof(completion_msg), 
                "전체 실행 완료: %d단계 실행 (PC: %d -> %d)", step_count, initial_pc, regs->pc);
    }
    
    ws_send_execution_step(completion_msg, NULL, 0);
    ws_send_ack(completion_msg);
    
    printf("전체 실행 성공: %d단계 실행\n", step_count);
    return 0;
}

// WebSocket 프로토콜 콜백
static int callback_cpu_protocol(struct lws *wsi, enum lws_callback_reasons reason,
                                void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            add_client(wsi);
            ws_send_ack("연결됨");
            ws_send_cpu_state();
            break;
            
        case LWS_CALLBACK_CLOSED:
            remove_client(wsi);
            break;
            
        case LWS_CALLBACK_RECEIVE: {
            char *message = malloc(len + 1);
            if (message) {
                memcpy(message, in, len);
                message[len] = '\0';
                
                // JSON 파싱
                json_object *root = json_tokener_parse(message);
                if (root) {
                    json_object *type_obj;
                    if (json_object_object_get_ex(root, "type", &type_obj)) {
                        const char *type = json_object_get_string(type_obj);
                        
                        if (strcmp(type, "assembly") == 0) {
                            json_object *payload_obj;
                            if (json_object_object_get_ex(root, "payload", &payload_obj)) {
                                const char *assembly = json_object_get_string(payload_obj);
                                ws_handle_assembly_code(assembly);
                            }
                        } else if (strcmp(type, "load_program") == 0) {
                            json_object *payload_obj;
                            if (json_object_object_get_ex(root, "payload", &payload_obj)) {
                                const char *program = json_object_get_string(payload_obj);
                                ws_handle_program_load(program);
                            }
                        } else if (strcmp(type, "step") == 0) {
                            ws_handle_step_execution();
                        } else if (strcmp(type, "reset") == 0) {
                            ws_handle_cpu_reset();
                        } else if (strcmp(type, "run_all") == 0) {
                            ws_handle_run_all();
                        } else if (strcmp(type, "get_state") == 0) {
                            ws_send_cpu_state();
                        } else if (strcmp(type, "get_memory") == 0) {
                            ws_send_memory_state();
                        } else if (strcmp(type, "get_cache") == 0) {
                            ws_send_cache_state();
                        } else if (strcmp(type, "ping") == 0) {
                            json_object *pong_msg = json_object_new_object();
                            json_object *pong_type = json_object_new_string("pong");
                            json_object_object_add(pong_msg, "type", pong_type);
                            
                            const char *pong_str = json_object_to_json_string(pong_msg);
                            size_t pong_len = strlen(pong_str);
                            unsigned char *buf = malloc(LWS_PRE + pong_len);
                            
                            if (buf) {
                                memcpy(&buf[LWS_PRE], pong_str, pong_len);
                                lws_write(wsi, &buf[LWS_PRE], pong_len, LWS_WRITE_TEXT);
                                free(buf);
                            }
                            
                            json_object_put(pong_msg);
                        }
                    }
                    json_object_put(root);
                }
                free(message);
            }
            break;
        }
        
        default:
            break;
    }
    
    return 0;
}

// 서버 실행
int ws_server_run(void) {
    while (server_running) {
        lws_service(server_ctx.context, 50);
    }
    return 0;
} 