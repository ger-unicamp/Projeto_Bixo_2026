# Boilerplate: Robô Móvel Diferencial

Este repositório contém o código-base em C++ (Arduino/PlatformIO) para o desenvolvimento do Robô Móvel Diferencial durante as **Oficinas de Robótica Prática**. 

A arquitetura do código foi projetada para sistemas de tempo real, utilizando interrupções de hardware para a leitura dos *encoders* e um *loop* principal não-bloqueante (sem a função `delay()`), operando a uma frequência fixa de 20Hz (50ms). Isso garante a estabilidade da malha de controle PID e a futura integração com o ecossistema **ROS 2**.

---

## Arquitetura de Hardware (Pinagem)
O código está pré-configurado para a placa **ESP32-C3 Super Mini**. Caso a equipe opte por utilizar outra versão do ESP32, as portas abaixo deverão ser alteradas.

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

---

## O que a equipe precisa desenvolver? (TODOs)
O código fornecido é um esqueleto funcional, mas a "inteligência" do robô precisará ser codificada pela equipe ao longo das 5 oficinas. Procurem pelas marcações `TODO` no código:

* **[Aula 2] Setup da Ponte H:** Configurar os pinos da Ponte H como `OUTPUT` e testar a geração de sinal PWM.
* **[Aula 3] Quadratura dos Encoders:** Alterar as funções `isr_encoder_esq()` e `isr_encoder_dir()` para lerem o estado do Pino B e contarem os *ticks* de forma bidirecional (frente e trás).
* **[Aula 3] Cinemática e Odometria:** Na função `calcula_odometria()`, implementar a matemática que converte *ticks* em Velocidade Angular (rad/s) e Linear (m/s).
* **[Aula 4] Controlador PID:** Na função `controle_pid()`, escrever o algoritmo de malha fechada (Proporcional, Integral e Derivativo) para estabilizar a velocidade das rodas.
* **[Aula 5] Integração micro-ROS:** Descomentar os cabeçalhos de `#include`, inicializar o nó do ROS 2 no `setup()` e preencher a mensagem de Odometria para enviar os dados da bancada para o PC.

---

## Critérios de Avaliação e Boas Práticas
Lembre-se que **20% da nota** das entregas está atrelada às Boas Práticas de Equipe.
1. **Controle de Versão:** Não utilizem pendrives ou envio de código por WhatsApp. Usem o `git pull` e `git push` regularmente.
2. **Commits Claros:** Evitem mensagens como *"atualizei código"*. Prefira mensagens descritivas como *"feat: implementação da leitura bidirecional do encoder esquerdo"*.
3. **Documentação Viva:** Atualizem este `README.md` ao longo das semanas. Se alterarem algum pino no projeto físico, documentem a mudança aqui imediatamente.

---
*Bons códigos e nos vemos na arena final!* 🏁
