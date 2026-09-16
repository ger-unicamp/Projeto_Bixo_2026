#include <Arduino.h>
#include <WiFi.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <geometry_msgs/msg/point.h>

#include "secrets.h"

// ==========================================
// 1. DEFINICAO DE PINOS (O grupo deve adaptar para o seu Hardware)
// ==========================================
// Pinos dos Encoders (Escolher pinos que suportam interrupcao de hardware)
#define ENC_IN_ESQ_A 0
#define ENC_IN_ESQ_B 1
#define ENC_IN_DIR_A 2
#define ENC_IN_DIR_B 3

// Pinos da Ponte H
#define MOT_ESQ_PWM 4
#define MOT_ESQ_IN1 5
#define MOT_ESQ_IN2 6
#define MOT_DIR_PWM 7
#define MOT_DIR_IN1 8
#define MOT_DIR_IN2 9

// ==========================================
// 1B. GEOMETRIA DO ROBO (AJUSTAR conforme o chassi do time)
// ==========================================
const float DISTANCIA_ENTRE_RODAS_M = 0.15f; // "L", distancia entre as rodas esquerda/direita (m)
const float RAIO_RODA_M = 0.03f;             // "R", raio da roda (m)

// Limites de seguranca para os comandos de velocidade (AJUSTAR conforme o robo)
const float VEL_LINEAR_MAX_MS = 0.3f;   // m/s
const float VEL_ANGULAR_MAX_RADS = 2.0f; // rad/s

// Ganhos do controlador proporcional que leva o robo ate uma coordenada (ver secao 6)
const float KP_LINEAR = 0.6f;
const float KP_ANGULAR = 1.5f;

// ==========================================
// 2. VARIAVEIS GLOBAIS DE SISTEMA
// ==========================================
// O termo 'volatile' informa ao compilador que a variavel pode mudar a qualquer momento
// fora do fluxo normal do codigo (ou seja, dentro das interrupcoes).
volatile long ticks_esq = 0;
volatile long ticks_dir = 0;

// Velocidade angular de cada roda (rad/s) que o controle_pid() deve perseguir.
// Atualizadas pelos callbacks do micro-ROS (cmd_vel ou bixo/goal), ver secoes 5 e 6.
volatile float setpoint_omega_esq = 0.0f;
volatile float setpoint_omega_dir = 0.0f;

// Variaveis para garantir que o loop principal rode em frequencia fixa (Sem delay!)
unsigned long tempo_anterior = 0;
const int INTERVALO_AMOSTRAGEM_MS = 50; // Roda o controle a 20Hz

// ==========================================
// 3. MICRO-ROS
// ==========================================
// Handles do rcl/rclc. O transporte usado e WiFi/UDP (ver README e ./bixo wifi),
// falando com o micro-ROS agent (docker) configurado em docker-compose.yml.
rcl_subscription_t cmd_vel_subscriber;
rcl_subscription_t goal_subscriber;
geometry_msgs__msg__Twist cmd_vel_msg;
geometry_msgs__msg__Point goal_msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

#define RCCHECK(fn)                     \
  {                                     \
    rcl_ret_t rc = fn;                  \
    if (rc != RCL_RET_OK) error_loop(); \
  }
#define RCSOFTCHECK(fn) \
  {                     \
    rcl_ret_t rc = fn;  \
    (void)rc;           \
  }

void error_loop() {
  while (true) {
    Serial.println("error_loop: rcl init falhou");
    delay(500);
  }
}

// ==========================================
// 4. INTERRUPCOES DE HARDWARE (ISRs)
// ==========================================
// IRAM_ATTR aloca a funcao na memoria RAM do microcontrolador, garantindo execucao extremamente rapida.
void IRAM_ATTR isr_encoder_esq() {
  // TODO (Aula 3): Voces precisam ler o estado do pino B para determinar
  // se a roda esta indo para frente ou para tras.
  // Por enquanto, o codigo apenas conta para cima:
  ticks_esq++;
}

void IRAM_ATTR isr_encoder_dir() {
  // TODO (Aula 3): Implementar a logica de quadratura para a roda direita.
  ticks_dir++;
}

// ==========================================
// 5. FUNcOES DE CALCULO E CONTROLE (AULAS 3 E 4)
// ==========================================
void calcula_odometria() {
  // O resgate de variaveis volatile precisa ser rapido.
  // Desligamos as interrupcoes por um microssegundo para copiar os valores e nao corromper os dados.
  noInterrupts();
  long ticks_atuais_esq = ticks_esq;
  long ticks_atuais_dir = ticks_dir;
  interrupts();

  // TODO (Aula 3): Com os ticks atuais e o tempo percorrido (INTERVALO_AMOSTRAGEM_MS),
  // calculem a Velocidade Angular de cada roda (rad/s).
  // Em seguida, calculem a Velocidade Linear (m/s) e Angular (rad/s) do centro do robo.
}

void controle_pid() {
  // TODO (Aula 4): Implementar o Controle em Malha Fechada, usando
  // setpoint_omega_esq / setpoint_omega_dir (rad/s, ver secao 6) como referencia:
  // 1. Calcular o Erro (setpoint_omega_* - Velocidade Real da roda, vinda da odometria)
  // 2. Calcular as acoes Proporcional (P), Integral (I) e Derivativa (D)
  // 3. Converter o resultado matematico em sinal PWM (0 a 255) para enviar para a Ponte H.
}

// ==========================================
// 6. CINEMATICA DIFERENCIAL + INTEGRACAO ROS 2
// ==========================================
// Converte uma velocidade linear (v, m/s) e angular (w, rad/s) do centro do robo
// nas velocidades angulares (rad/s) de cada roda e atualiza os setpoints do controle_pid().
void aplica_velocidade(float v, float w) {
  v = constrain(v, -VEL_LINEAR_MAX_MS, VEL_LINEAR_MAX_MS);
  w = constrain(w, -VEL_ANGULAR_MAX_RADS, VEL_ANGULAR_MAX_RADS);

  float vel_linear_esq = v - (w * DISTANCIA_ENTRE_RODAS_M / 2.0f);
  float vel_linear_dir = v + (w * DISTANCIA_ENTRE_RODAS_M / 2.0f);

  setpoint_omega_esq = vel_linear_esq / RAIO_RODA_M;
  setpoint_omega_dir = vel_linear_dir / RAIO_RODA_M;
}

// Assina geometry_msgs/Twist em "cmd_vel": usa linear.x como v e angular.z como w.
void cmd_vel_callback(const void *msgin) {
  const geometry_msgs__msg__Twist *msg = (const geometry_msgs__msg__Twist *)msgin;
  aplica_velocidade((float)msg->linear.x, (float)msg->angular.z);
}

// Calcula (v, w) para levar o robo ate uma coordenada (x, y) no referencial do robo,
// simplificando o robo na origem (0, 0) virado para o eixo Y positivo (Y = frente, X = lateral).
// Controlador proporcional simples: gira em direcao ao alvo e anda proporcional a distancia.
// Nao substitui um planejador de trajetoria de verdade, e um ponto de partida.
void coordenada_para_velocidade(float x, float y, float *v, float *w) {
  float distancia = sqrtf(x * x + y * y);
  float angulo_para_alvo = atan2f(x, y); // 0 = alvo na frente (+Y), >0 = alvo a direita

  *w = KP_ANGULAR * angulo_para_alvo;
  *v = KP_LINEAR * distancia;

  // Gira no lugar primeiro se o alvo estiver muito fora do eixo de frente,
  // so anda pra frente depois de estar (quase) apontado pra ele.
  if (fabsf(angulo_para_alvo) > 0.3f) {
    *v = 0.0f;
  }
}

// Assina geometry_msgs/Point em "bixo/goal": trata (x, y) como coordenada-alvo (ver acima) e
// converte direto em setpoints de roda, reaproveitando aplica_velocidade().
void goal_callback(const void *msgin) {
  const geometry_msgs__msg__Point *msg = (const geometry_msgs__msg__Point *)msgin;
  float v, w;
  coordenada_para_velocidade((float)msg->x, (float)msg->y, &v, &w);
  aplica_velocidade(v, w);
}

// ==========================================
// SETUP INICIAL
// ==========================================
void setup() {
  Serial.begin(115200);

  // Configuracao dos pinos dos encoders (INPUT_PULLUP previne ruidos caso o encoder seja do tipo open-collector)
  pinMode(ENC_IN_ESQ_A, INPUT_PULLUP);
  pinMode(ENC_IN_ESQ_B, INPUT_PULLUP);
  pinMode(ENC_IN_DIR_A, INPUT_PULLUP);
  pinMode(ENC_IN_DIR_B, INPUT_PULLUP);

  // Acoplando as interrupcoes aos pinos A de cada motor.
  // "RISING" significa que a interrupcao dispara quando o sinal sobe de 0V para 3.3V/5V.
  attachInterrupt(digitalPinToInterrupt(ENC_IN_ESQ_A), isr_encoder_esq, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_IN_DIR_A), isr_encoder_dir, RISING);

  // TODO (Aula 2): Configurar os pinos da Ponte H como saidas (OUTPUT) e configurar os canais PWM.

  // ==========================================
  // SETUP DO MICRO-ROS
  // ==========================================
  // Credenciais de wifi/agent ficam fora do repo: gere firmware/src/secrets.h com `./bixo wifi ...`
  // (veja firmware/src/secrets.h.example e o README).
  set_microros_wifi_transports((char *)WIFI_SSID, (char *)WIFI_PASS,
                                (char *)AGENT_IP, AGENT_PORT);
  WiFi.setSleep(false); // evita "could not send data: 12" (ENOMEM) por modem-sleep
  delay(2000);

  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "bixo_esp32c3_node", "", &support));

  RCCHECK(rclc_subscription_init_default(
      &cmd_vel_subscriber, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel"));

  RCCHECK(rclc_subscription_init_default(
      &goal_subscriber, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Point), "bixo/goal"));

  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &cmd_vel_subscriber, &cmd_vel_msg,
                                          &cmd_vel_callback, ON_NEW_DATA));
  RCCHECK(rclc_executor_add_subscription(&executor, &goal_subscriber, &goal_msg,
                                          &goal_callback, ON_NEW_DATA));

  // TODO (Aula 5): publicar odometria (nav_msgs/Odometry) com os dados de calcula_odometria(),
  // assim que ela estiver implementada, para o PC acompanhar a pose do robo.

  Serial.println("Sistema Iniciado. Aguardando inicio dos ciclos de controle...");
}

// ==========================================
// LOOP PRINCIPAL (Arquitetura Nao-Bloqueante)
// ==========================================
void loop() {
  unsigned long tempo_atual = millis();

  // Verifica se ja passou o tempo necessario (ex: 50ms) para rodar o controle novamente
  if (tempo_atual - tempo_anterior >= INTERVALO_AMOSTRAGEM_MS) {

    calcula_odometria();
    controle_pid();

    // (Uso para as Aulas 3 e 4) - Log Serial para os Engenheiros de Dados plotarem graficos!
    Serial.print("Ticks_Esq:");
    Serial.print(ticks_esq);
    Serial.print("\tTicks_Dir:");
    Serial.print(ticks_dir);
    Serial.print("\tSetpoint_Esq(rad/s):");
    Serial.print(setpoint_omega_esq);
    Serial.print("\tSetpoint_Dir(rad/s):");
    Serial.println(setpoint_omega_dir);

    // Atualiza o relogio para a proxima execucao
    tempo_anterior = tempo_atual;
  }

  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
}
