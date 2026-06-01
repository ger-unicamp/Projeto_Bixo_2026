#include <Arduino.h>

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
// 2. VARIAVEIS GLOBAIS DE SISTEMA
// ==========================================
// O termo 'volatile' informa ao compilador que a variavel pode mudar a qualquer momento 
// fora do fluxo normal do codigo (ou seja, dentro das interrupcoes).
volatile long ticks_esq = 0;
volatile long ticks_dir = 0;

// Variaveis para garantir que o loop principal rode em frequencia fixa (Sem delay!)
unsigned long tempo_anterior = 0;
const int INTERVALO_AMOSTRAGEM_MS = 50; // Roda o controle a 20Hz

// ==========================================
// 3. CABECALHOS PARA O MICRO-ROS (PARA A AULA 5)
// ==========================================
/* 
 * ATENCAO ALUNOS: Deixem esta secao comentada durante as Aulas 1 a 4.
 * Voces a ativarao na Aula 5 para transformar o microcontrolador em um No ROS 2.
 */
// #include <micro_ros_arduino.h>
// #include <rcl/rcl.h>
// #include <rclc/rclc.h>
// #include <rclc/executor.h>
// #include <geometry_msgs/msg/twist.h>
// #include <nav_msgs/msg/odometry.h>
// rcl_publisher_t odom_publisher;
// rcl_subscription_t cmd_vel_subscriber;

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
  // TODO (Aula 4): Implementar o Controle em Malha Fechada
  // 1. Calcular o Erro (Setpoint desejado - Velocidade Real das rodas)
  // 2. Calcular as acoes Proporcional (P), Integral (I) e Derivativa (D)
  // 3. Converter o resultado matematico em sinal PWM (0 a 255) para enviar para a Ponte H.
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

  // TODO: Configurar os pinos da Ponte H como saidas (OUTPUT) e configurar os canais PWM.
  
  // ==========================================
  // SETUP DO MICRO-ROS (Comentado para Aulas 1-4)
  // ==========================================
  // set_microros_transports();
  // rclc_support_init(...);
  // rclc_node_init_default(...);
  // rclc_publisher_init_default(...);
  
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
    
    // ==========================================
    // INTEGRAcaO ROS 2 (Apenas na Aula 5)
    // ==========================================
    // TODO: Popular a mensagem nav_msgs/Odometry com os dados calculados por voces e publicar:
    // rcl_publish(&odom_publisher, &odom_msg, NULL);
    // rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    
    // (Uso para as Aulas 3 e 4) - Log Serial para os Engenheiros de Dados plotarem graficos!
    Serial.print("Ticks_Esq:"); 
    Serial.print(ticks_esq);
    Serial.print("\tTicks_Dir:"); 
    Serial.println(ticks_dir);
    
    // Atualiza o relogio para a proxima execucao
    tempo_anterior = tempo_atual;
  }
}
