# Projeto Bixo 2026: Robô Móvel Diferencial

Este repositório contém o código-base em C++ (Arduino/PlatformIO) para o desenvolvimento do Robô Móvel Diferencial durante as **Oficinas de Robótica Prática**, com integração **micro-ROS** (agent em docker) já pronta via WiFi/UDP.

A arquitetura do código foi projetada para sistemas de tempo real, utilizando interrupções de hardware para a leitura dos *encoders* e um *loop* principal não-bloqueante (sem a função `delay()`), operando a uma frequência fixa de 20Hz (50ms). Isso garante a estabilidade da malha de controle PID e a integração com o ecossistema **ROS 2**.

O firmware fica em [`firmware/`](firmware/) (projeto PlatformIO); tudo é controlado pelo script [`./bixo`](bixo) na raiz.

---

## Requisitos

- docker + docker compose
- `pio` (PlatformIO Core): `pip install platformio` ou extensão VSCode
- ESP32-C3 Super Mini conectado por USB

## Uso

```
./bixo ip                                    # pega o IP do seu PC na rede
./bixo wifi "MINHA_REDE" "MINHA_SENHA" 192.168.x.x
./bixo agent up                              # sobe o agent (udp4:8888, host network)
./bixo fw run                                # compila, grava e abre monitor serial
```

O comando `./bixo wifi` gera `firmware/src/secrets.h` (fora do git, veja `firmware/src/secrets.h.example`) com as credenciais de WiFi e o IP/porta do agent. A placa precisa entrar na mesma rede WiFi do PC e falar UDP com o agent na porta 8888.

`./bixo agent up` também sobe um container `ros2-cli` (mesma rede do agent) só pra rodar comandos ROS 2 sem precisar instalar ROS no host.

## Mandando comandos pro robô

Não precisa saber `ros2 topic pub` nem os nomes dos tópicos — usa o `./bixo`:

```
./bixo cmd vel 0.1 0.0     # linear.x=0.1 m/s, angular.z=0.0 rad/s
./bixo cmd goal 0.2 1.0    # vai até (x=0.2, y=1.0) no referencial do robô
./bixo cmd stop            # equivale a `cmd vel 0 0`
```

- `cmd vel <v> [w]` publica em `cmd_vel` (`geometry_msgs/Twist`) — `linear.x` (m/s) e `angular.z` (rad/s) do centro do robô, convertidos em velocidade angular de cada roda (cinemática diferencial).
- `cmd goal <x> <y>` publica em `bixo/goal` (`geometry_msgs/Point`) — coordenada-alvo `(x, y)` no referencial do robô (robô na origem, virado para o eixo Y positivo); um controlador proporcional simples calcula `(v, w)` até o alvo e reaproveita a mesma conversão para roda.

Modo avançado (direto por trás do `./bixo cmd`, útil pra depurar): `docker compose exec ros2-cli ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.1}, angular: {z: 0.0}}"`.

---

## Arquitetura de Hardware (Pinagem)
O código está pré-configurado para a placa **ESP32-C3 Super Mini**. Caso a equipe opte por utilizar outra versão do ESP32, as portas abaixo deverão ser alteradas em `firmware/src/main.cpp`.

### Encoders (Motores DC)
| Componente | Pino ESP32-C3 | Função |
| :--- | :--- | :--- |
| Encoder Roda Esquerda (Canal A) | `GPIO 0` | Interrupção (Tick) |
| Encoder Roda Esquerda (Canal B) | `GPIO 1` | Direção (Quadratura) |
| Encoder Roda Direita (Canal A) | `GPIO 2` | Interrupção (Tick) |
| Encoder Roda Direita (Canal B) | `GPIO 3` | Direção (Quadratura) |

### Driver de Potência (Ponte H)
| Componente | Pino ESP32-C3 | Função |
| :--- | :--- | :--- |
| Motor Esquerdo (PWM) | `GPIO 4` | Controle de Velocidade |
| Motor Esquerdo (IN1 / IN2) | `GPIO 5`, `GPIO 6` | Controle de Sentido |
| Motor Direito (PWM) | `GPIO 7` | Controle de Velocidade |
| Motor Direito (IN1 / IN2) | `GPIO 8`, `GPIO 9` | Controle de Sentido |

### Geometria do robô

`DISTANCIA_ENTRE_RODAS_M` e `RAIO_RODA_M`, no topo de `firmware/src/main.cpp`, **devem ser ajustados** para as medidas reais do chassi de cada time — são usados na conversão de `cmd_vel`/`bixo/goal` para velocidade de roda.

---

## O que a equipe precisa desenvolver? (TODOs)
O código fornecido é um esqueleto funcional — a comunicação micro-ROS já está de pé — mas a "inteligência" do robô ainda precisa ser codificada pela equipe ao longo das oficinas. Procurem pelas marcações `TODO` no código:

* **[Aula 2] Setup da Ponte H:** Configurar os pinos da Ponte H como `OUTPUT` e testar a geração de sinal PWM.
* **[Aula 3] Quadratura dos Encoders:** Alterar as funções `isr_encoder_esq()` e `isr_encoder_dir()` para lerem o estado do Pino B e contarem os *ticks* de forma bidirecional (frente e trás).
* **[Aula 3] Cinemática e Odometria:** Na função `calcula_odometria()`, implementar a matemática que converte *ticks* em Velocidade Angular (rad/s) e Linear (m/s).
* **[Aula 4] Controlador PID:** Na função `controle_pid()`, escrever o algoritmo de malha fechada (Proporcional, Integral e Derivativo) que persegue `setpoint_omega_esq`/`setpoint_omega_dir` (já preenchidos pelos tópicos ROS) e gera o PWM da Ponte H.
* **[Aula 5] Odometria via micro-ROS:** Publicar `nav_msgs/Odometry` com os dados de `calcula_odometria()`, assim que ela estiver implementada, para o PC acompanhar a pose do robô.

---

## Critérios de Avaliação e Boas Práticas
Lembre-se que **20% da nota** das entregas está atrelada às Boas Práticas de Equipe.
1. **Controle de Versão:** Não utilizem pendrives ou envio de código por WhatsApp. Usem o `git pull` e `git push` regularmente.
2. **Commits Claros:** Evitem mensagens como *"atualizei código"*. Prefira mensagens descritivas como *"feat: implementação da leitura bidirecional do encoder esquerdo"*.
3. **Documentação Viva:** Atualizem este `README.md` ao longo das semanas. Se alterarem algum pino no projeto físico, documentem a mudança aqui imediatamente.

---

## Sem WiFi / ESP-NOW depois

Setup atual é 1 placa, WiFi/UDP direto no agent — sem ESP-NOW. Se depois quiser adicionar uma segunda placa em ESP-NOW (nó remoto sem WiFi), essa placa vira "bridge": recebe ESP-NOW e repassa pro micro-ROS via WiFi/UDP.

---
*Bons códigos e nos vemos na arena final!* 🏁
