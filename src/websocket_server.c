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

/*
 * @brief CPU 프로토콜 콜백 함수
 * @param wsi WebSocket 인스턴스
 * @param reason 콜백 이유
 * @param user 사용자 데이터
 * @param in 입력 데이터
 * @param len 데이터 길이
 * @returns 콜백 처리 결과 (int)
 */
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

/*
 * @brief WebSocket 서버를 초기화합니다
 * @param port 서버 포트 번호
 * @returns 초기화 성공 시 0, 실패 시 -1
 */
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

/*
 * @brief WebSocket 서버를 정리합니다
 * @param 없음
 * @returns 없음 (void)
 */
void ws_server_cleanup(void) {
    server_running = 0;
    if (server_ctx.context) {
        lws_context_destroy(server_ctx.context);
        server_ctx.context = NULL;
    }
    pthread_mutex_destroy(&server_ctx.mutex);
}

/*
 * @brief 새로운 클라이언트를 추가합니다
 * @param wsi WebSocket 인스턴스
 * @returns 클라이언트 ID, 실패 시 -1
 */
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

/*
 * @brief 클라이언트를 제거합니다
 * @param wsi WebSocket 인스턴스
 * @returns 없음 (void)
 */
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

/*
 * @brief 모든 클라이언트에게 메시지를 전송합니다
 * @param message 전송할 메시지
 * @returns 없음 (void)
 */
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

/*
 * @brief 레지스터 이름을 번호로 변환합니다
 * @param reg_str 레지스터 문자열 (예: "R1")
 * @returns 레지스터 번호 (1-7), 실패 시 -1
 */
int parse_register(const char* reg_str) {
    if (reg_str[0] == 'R' && strlen(reg_str) == 2) {
        int reg_num = reg_str[1] - '0';
        if (reg_num >= 1 && reg_num <= 7) {
            return reg_num; // R1=1, R2=2, ..., R7=7
        }
    }
    return -1; // 레지스터가 아님
}

/*
 * @brief 어셈블리 코드를 바이트로 변환합니다
 * @param assembly 어셈블리 코드 문자열
 * @param output_bytes 출력 바이트 배열
 * @param max_length 최대 출력 길이
 * @returns 생성된 바이트 수, 실패 시 0
 */
int decode_assembly_to_bytes(const char* assembly, uint8_t* output_bytes, int max_length) {
    if (!assembly || !output_bytes || max_length < 2) {
        return 0;
    }
    
    char instruction[32];
    char operand1_str[32], operand2_str[32];
    
    int parsed = sscanf(assembly, "%s %31[^,], %31s", instruction, operand1_str, operand2_str);
    
    if (parsed < 2) {
        printf("❌ 파싱 실패: %s\n", assembly);
        return 0;
    }
    
    uint8_t opcode = 0;
    if (strcmp(instruction, "ADD") == 0) opcode = 0;
    else if (strcmp(instruction, "SUB") == 0) opcode = 1;
    else if (strcmp(instruction, "MUL") == 0) opcode = 2;
    else if (strcmp(instruction, "DIV") == 0) opcode = 3;
    else if (strcmp(instruction, "MOV") == 0) opcode = 4;
    else {
        printf("❌ 알 수 없는 명령어: %s\n", instruction);
        return 0;
    }
    
    // 🎯 MOV 명령어 특별 처리 (레지스터 + 8비트 즉시값 지원)
    if (opcode == 4) {
        // MOV 레지스터, 즉시값 형태 처리
        if (operand1_str[0] == 'R' && strlen(operand1_str) == 2) {
            int reg_num = operand1_str[1] - '0';
            if (reg_num >= 1 && reg_num <= 7) {
                int immediate_val = atoi(operand2_str);
                if (immediate_val < 0 || immediate_val > 255) {
                    printf("❌ 즉시값 범위 오류 (0-255): %d\n", immediate_val);
                    return 0;
                }
                
                // MOV 레지스터, 즉시값: 4비트 opcode + 4비트 레지스터 + 8비트 즉시값
                uint16_t instruction_word = (opcode << 12) | (reg_num << 8) | (immediate_val & 0xFF);
                
                output_bytes[0] = (instruction_word >> 8) & 0xFF;
                output_bytes[1] = instruction_word & 0xFF;
                
                printf("🎯 MOV 인코딩: %s -> 레지스터=%d, 즉시값=%d -> 바이트: 0x%02X 0x%02X\n", 
                       assembly, reg_num, immediate_val, output_bytes[0], output_bytes[1]);
                
                return 2;
            } else {
                printf("❌ 잘못된 레지스터: %s\n", operand1_str);
                return 0;
            }
        }
        // MOV 메모리, 즉시값 형태는 기존 방식 사용
        else {
            uint8_t reg1_val = atoi(operand1_str);
            uint8_t reg2_val = (parsed == 3) ? atoi(operand2_str) : 0;
            
            if (reg1_val > 63) reg1_val = 63;
            if (reg2_val > 63) reg2_val = 63;
            
            uint16_t instruction_word = (opcode << 12) | ((reg1_val & 0x3F) << 6) | (reg2_val & 0x3F);
            
            output_bytes[0] = (instruction_word >> 8) & 0xFF;
            output_bytes[1] = instruction_word & 0xFF;
            
            printf("📊 MOV 메모리 인코딩: %s -> 바이트: 0x%02X 0x%02X\n", assembly, output_bytes[0], output_bytes[1]);
            return 2;
        }
    }
    
    // 🚀 ADD/SUB/MUL/DIV 명령어 - 레지스터 지원 향상
    if (opcode >= 0 && opcode <= 3) {
        // 두 피연산자가 모두 레지스터인지 확인
        if (operand1_str[0] == 'R' && operand2_str[0] == 'R' && 
            strlen(operand1_str) == 2 && strlen(operand2_str) == 2) {
            
            int reg1_num = operand1_str[1] - '0';
            int reg2_num = operand2_str[1] - '0';
            
            if (reg1_num >= 1 && reg1_num <= 7 && reg2_num >= 1 && reg2_num <= 7) {
                // 새로운 레지스터 포맷: 4비트 opcode + 4비트 reg1 + 4비트 reg2 + 4비트 플래그(1111)
                uint16_t instruction_word = (opcode << 12) | (reg1_num << 8) | (reg2_num << 4) | 0xF;
                
                output_bytes[0] = (instruction_word >> 8) & 0xFF;
                output_bytes[1] = instruction_word & 0xFF;
                
                printf("🚀 ALU 레지스터 인코딩: %s -> reg1=%d, reg2=%d -> 바이트: 0x%02X 0x%02X\n", 
                       assembly, reg1_num, reg2_num, output_bytes[0], output_bytes[1]);
                
                return 2;
            }
        }
    }
    
    // 다른 명령어들 (기존 방식)
    uint8_t reg1_val, reg2_val = 0;
    
    // 첫 번째 피연산자: 레지스터인지 확인
    if (operand1_str[0] == 'R' && strlen(operand1_str) == 2) {
        int reg_num = operand1_str[1] - '0';
        if (reg_num >= 1 && reg_num <= 7) {
            reg1_val = 100 + reg_num;  // R1=101, R2=102, ..., R7=107
            printf("🎯 첫 번째: %s -> 인코딩 %d\n", operand1_str, reg1_val);
        } else {
            printf("❌ 잘못된 레지스터: %s\n", operand1_str);
            return 0;
        }
    } else {
        reg1_val = atoi(operand1_str);
        if (reg1_val > 63) reg1_val = 63;
        printf("📊 첫 번째: 즉시값 %d\n", reg1_val);
    }
    
    // 두 번째 피연산자
    if (parsed == 3) {
        if (operand2_str[0] == 'R' && strlen(operand2_str) == 2) {
            int reg_num = operand2_str[1] - '0';
            if (reg_num >= 1 && reg_num <= 7) {
                reg2_val = 100 + reg_num;
                printf("🎯 두 번째: %s -> 인코딩 %d\n", operand2_str, reg2_val);
            } else {
                printf("❌ 잘못된 레지스터: %s\n", operand2_str);
                return 0;
            }
        } else {
            reg2_val = atoi(operand2_str);
            if (reg2_val > 63) reg2_val = 63;
            printf("📊 두 번째: 즉시값 %d\n", reg2_val);
        }
    }
    
    // 인코딩
    uint16_t instruction_word = (opcode << 12) | ((reg1_val & 0x3F) << 6) | (reg2_val & 0x3F);
    
    output_bytes[0] = (instruction_word >> 8) & 0xFF;
    output_bytes[1] = instruction_word & 0xFF;
    
    printf("파싱 성공: %s -> 바이트: 0x%02X 0x%02X\n", assembly, output_bytes[0], output_bytes[1]);
    
    return 2;
}

/*
 * @brief 바이트를 어셈블리 코드로 변환합니다
 * @param bytes 입력 바이트 배열
 * @param byte_count 바이트 개수
 * @param output_assembly 출력 어셈블리 문자열
 * @param max_length 최대 출력 길이
 * @returns 변환 성공 시 1, 실패 시 0
 */
int decode_bytes_to_assembly(const uint8_t* bytes, int byte_count, char* output_assembly, int max_length) {
    if (!bytes || byte_count < 2 || !output_assembly || max_length < 32) {
        return 0;
    }
    
    uint16_t instruction_word = (bytes[0] << 8) | bytes[1];
    
    // 4비트 opcode 추출
    uint8_t opcode = (instruction_word >> 12) & 0xF;
    
    const char* op_name;
    switch (opcode) {
        case 0: op_name = "ADD"; break;
        case 1: op_name = "SUB"; break;
        case 2: op_name = "MUL"; break;
        case 3: op_name = "DIV"; break;
        case 4: op_name = "MOV"; break;
        default: return 0;
    }
    
    // 🎯 MOV 명령어 특별 처리
    if (opcode == 4) {
        // MOV 레지스터, 즉시값: 4비트 opcode + 4비트 레지스터 + 8비트 즉시값
        uint8_t reg_num = (instruction_word >> 8) & 0xF;
        uint8_t immediate_val = instruction_word & 0xFF;
        
        // 레지스터 번호가 1-7 범위인지 확인
        if (reg_num >= 1 && reg_num <= 7) {
            snprintf(output_assembly, max_length, "%s R%d, %d", op_name, reg_num, immediate_val);
            printf("🎯 MOV 디코딩: 바이트 0x%02X 0x%02X -> %s\n", bytes[0], bytes[1], output_assembly);
            return 1;
        }
        // 기존 방식 (메모리 주소)으로 디코딩 시도
        else {
            uint8_t reg1_val = (instruction_word >> 6) & 0x3F;
            uint8_t reg2_val = instruction_word & 0x3F;
            
            char reg1_str[16], reg2_str[16];
            
            if (reg1_val > 100) {
                snprintf(reg1_str, sizeof(reg1_str), "R%d", reg1_val - 100);
            } else {
                snprintf(reg1_str, sizeof(reg1_str), "%d", reg1_val);
            }
            
            if (reg2_val > 100) {
                snprintf(reg2_str, sizeof(reg2_str), "R%d", reg2_val - 100);
            } else {
                snprintf(reg2_str, sizeof(reg2_str), "%d", reg2_val);
            }
            
            snprintf(output_assembly, max_length, "%s %s, %s", op_name, reg1_str, reg2_str);
            printf("📊 MOV 메모리 디코딩: 바이트 0x%02X 0x%02X -> %s\n", bytes[0], bytes[1], output_assembly);
            return 1;
        }
    }
    
    // 다른 명령어들 (기존 방식): 4비트 opcode + 6비트 reg1 + 6비트 reg2
    uint8_t reg1_val = (instruction_word >> 6) & 0x3F;
    uint8_t reg2_val = instruction_word & 0x3F;
    
    char reg1_str[16], reg2_str[16];
    
    if (reg1_val > 100) {
        snprintf(reg1_str, sizeof(reg1_str), "R%d", reg1_val - 100);
    } else {
        snprintf(reg1_str, sizeof(reg1_str), "%d", reg1_val);
    }
    
    if (reg2_val > 100) {
        snprintf(reg2_str, sizeof(reg2_str), "R%d", reg2_val - 100);
    } else {
        snprintf(reg2_str, sizeof(reg2_str), "%d", reg2_val);
    }
    
    snprintf(output_assembly, max_length, "%s %s, %s", op_name, reg1_str, reg2_str);
    return 1;
}

/*
 * @brief CPU 상태 JSON 메시지를 생성합니다
 * @param 없음
 * @returns JSON 객체 포인터
 */
json_object* create_state_message(void) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("state");
    json_object *payload = json_object_new_object();
    
    CPU_Registers *regs = get_cpu_registers();
    
    json_object *pc = json_object_new_int(regs->pc);
    json_object *reg1 = json_object_new_int(regs->register1);
    json_object *reg2 = json_object_new_int(regs->register2);
    json_object *reg3 = json_object_new_int(regs->register3);
    json_object *reg4 = json_object_new_int(regs->register4);
    json_object *reg5 = json_object_new_int(regs->register5);
    json_object *reg6 = json_object_new_int(regs->register6);
    json_object *reg7 = json_object_new_int(regs->register7);
    json_object *overflow_flag = json_object_new_boolean(regs->overflow_flag);
    
    json_object_object_add(payload, "pc", pc);
    json_object_object_add(payload, "register1", reg1);
    json_object_object_add(payload, "register2", reg2);
    json_object_object_add(payload, "register3", reg3);
    json_object_object_add(payload, "register4", reg4);
    json_object_object_add(payload, "register5", reg5);
    json_object_object_add(payload, "register6", reg6);
    json_object_object_add(payload, "register7", reg7);
    json_object_object_add(payload, "overflow_flag", overflow_flag);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

/*
 * @brief 메모리 상태 JSON 메시지를 생성합니다
 * @param 없음
 * @returns JSON 객체 포인터
 */
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

/*
 * @brief 캐시 상태 JSON 메시지를 생성합니다
 * @param 없음
 * @returns JSON 객체 포인터
 */
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

/*
 * @brief 실행 단계 JSON 메시지를 생성합니다
 * @param instruction 실행된 명령어 문자열
 * @param bytes 명령어 바이트 배열
 * @param byte_count 바이트 개수
 * @returns JSON 객체 포인터
 */
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

/*
 * @brief 확인 JSON 메시지를 생성합니다
 * @param message 확인 메시지 문자열
 * @returns JSON 객체 포인터
 */
json_object* create_ack_message(const char* message) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("ack");
    json_object *payload = json_object_new_string(message);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

/*
 * @brief 에러 JSON 메시지를 생성합니다
 * @param error 에러 메시지 문자열
 * @returns JSON 객체 포인터
 */
json_object* create_error_message(const char* error) {
    json_object *root = json_object_new_object();
    json_object *type = json_object_new_string("error");
    json_object *payload = json_object_new_string(error);
    
    json_object_object_add(root, "type", type);
    json_object_object_add(root, "payload", payload);
    
    return root;
}

// 메시지 전송 함수들
/*
 * @brief CPU 상태를 모든 클라이언트에 전송합니다
 * @param 없음
 * @returns 없음 (void)
 */
void ws_send_cpu_state(void) {
    json_object *msg = create_state_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 메모리 상태를 모든 클라이언트에 전송합니다
 * @param 없음
 * @returns 없음 (void)
 */
void ws_send_memory_state(void) {
    json_object *msg = create_memory_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 캐시 상태를 모든 클라이언트에 전송합니다
 * @param 없음
 * @returns 없음 (void)
 */
void ws_send_cache_state(void) {
    json_object *msg = create_cache_message();
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 실행 단계 정보를 모든 클라이언트에 전송합니다
 * @param instruction 실행된 명령어 문자열
 * @param bytes 명령어 바이트 배열
 * @param byte_count 바이트 개수
 * @returns 없음 (void)
 */
void ws_send_execution_step(const char* instruction, const uint8_t* bytes, int byte_count) {
    json_object *msg = create_execution_message(instruction, bytes, byte_count);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 확인 메시지를 모든 클라이언트에 전송합니다
 * @param message 확인 메시지 문자열
 * @returns 없음 (void)
 */
void ws_send_ack(const char* message) {
    json_object *msg = create_ack_message(message);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 에러 메시지를 모든 클라이언트에 전송합니다
 * @param error_msg 에러 메시지 문자열
 * @returns 없음 (void)
 */
void ws_send_error(const char* error_msg) {
    json_object *msg = create_error_message(error_msg);
    const char *json_str = json_object_to_json_string(msg);
    broadcast_message(json_str);
    json_object_put(msg);
}

/*
 * @brief 어셈블리 코드를 처리합니다
 * @param assembly_code 어셈블리 코드 문자열
 * @returns 처리 성공 시 0, 실패 시 -1
 */
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
/*
 * @brief 프로그램을 로드합니다
 * @param program_code 프로그램 코드 문자열
 * @returns 로드 성공 시 0, 실패 시 -1
 */
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
/*
 * @brief CPU를 한 단계 실행합니다
 * @param 없음
 * @returns 실행 성공 시 0, 실패 시 -1
 */
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
    
    // 실행 전 오버플로우 플래그 상태 저장
    bool prev_overflow_flag = get_overflow_flag(regs);
    
    // CPU 한 단계 실행
    cpu_step();
    
    // 실행 후 PC와 플래그 상태 확인
    regs = get_cpu_registers();
    bool current_overflow_flag = get_overflow_flag(regs);
    
    // 오버플로우 플래그 변화 감지 및 에러 메시지 전송
    if (current_overflow_flag && !prev_overflow_flag) {
        // 오버플로우 플래그가 새로 설정됨 - 오버플로우/언더플로우 발생
        if (strstr(current_instruction, "SUB") || strstr(current_instruction, "sub")) {
            ws_send_error("🚨 뺄셈 오버플로우 발생: 부호 있는 정수 범위를 벗어났습니다 (OF=1)");
        } else if (strstr(current_instruction, "ADD") || strstr(current_instruction, "add")) {
            ws_send_error("🚨 덧셈 오버플로우 발생: 부호 있는 정수 범위를 벗어났습니다 (OF=1)");
        } else if (strstr(current_instruction, "MUL") || strstr(current_instruction, "mul")) {
            ws_send_error("🚨 곱셈 오버플로우 발생: 부호 있는 정수 범위를 벗어났습니다 (OF=1)");
        } else if (strstr(current_instruction, "DIV") || strstr(current_instruction, "div")) {
            ws_send_error("🚨 나눗셈 에러가 발생했습니다 (OF=1)");
        }
    }
    
    // 실행된 명령어 정보 전송
    char step_msg[256];
    const char* flag_status = current_overflow_flag ? " [OF=1]" : " [OF=0]";
    snprintf(step_msg, sizeof(step_msg), "실행: %s | PC: %d -> %d%s", 
             current_instruction, prev_pc, regs->pc, flag_status);
    
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
    
    // 완료 메시지에 플래그 상태 포함
    char completion_msg[128];
    if (current_overflow_flag) {
        snprintf(completion_msg, sizeof(completion_msg), "단계 실행 완료 (경고: 오버플로우 플래그 설정됨)");
    } else {
        snprintf(completion_msg, sizeof(completion_msg), "단계 실행 완료");
    }
    ws_send_ack(completion_msg);
    printf("단계 실행 성공: %s | PC %d -> %d\n", current_instruction, prev_pc, regs->pc);
    
    return 0;
}

// CPU 리셋 처리
/*
 * @brief CPU를 리셋합니다
 * @param 없음
 * @returns 리셋 성공 시 0
 */
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
/*
 * @brief CPU를 모든 명령어가 완료될 때까지 실행합니다
 * @param 없음
 * @returns 실행 성공 시 0, 실패 시 -1
 */
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

// 단일 명령어 로드 및 실행 준비
/*
 * @brief 단일 명령어를 로드합니다
 * @param assembly_code 어셈블리 코드 문자열
 * @returns 로드 성공 시 0, 실패 시 -1
 */
int ws_handle_single_instruction_load(const char* assembly_code) {
    printf("단일 명령어 로드 요청: %s\n", assembly_code);
    
    // CPU 리셋 (이전 상태 초기화)
    cpu_reset();
    
    // 어셈블리 코드를 바이트로 변환
    uint8_t bytes[256];
    int byte_count = decode_assembly_to_bytes(assembly_code, bytes, sizeof(bytes));
    
    if (byte_count <= 0) {
        ws_send_error("명령어 파싱 실패");
        return -1;
    }
    
    // 메모리 시작 부분에 명령어 로드
    Memory *memory = get_cpu_memory();
    for (int i = 0; i < byte_count && i < MEMORY_SIZE; i++) {
        memory->data[i] = bytes[i];
    }
    
    // PC를 0으로 설정 (명령어 시작 위치)
    CPU_Registers *regs = get_cpu_registers();
    regs->pc = 0;
    
    printf("단일 명령어 로드 완료: %s (%d바이트)\n", assembly_code, byte_count);
    
    // 상태 전송
    ws_send_cpu_state();
    ws_send_memory_state();
    ws_send_ack("단일 명령어 로드 완료");
    
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
                        } else if (strcmp(type, "load_single_instruction") == 0) {
                            json_object *payload_obj;
                            if (json_object_object_get_ex(root, "payload", &payload_obj)) {
                                const char *instruction = json_object_get_string(payload_obj);
                                ws_handle_single_instruction_load(instruction);
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
/*
 * @brief WebSocket 서버를 실행합니다
 * @param 없음
 * @returns 서버 종료 상태 (int)
 */
int ws_server_run(void) {
    while (server_running) {
        lws_service(server_ctx.context, 50);
    }
    return 0;
} 