# Esteira Inteligente com Raspberry Pi Pico W

Projeto de controle remoto via Wi-Fi para uma esteira inteligente, utilizando Raspberry Pi Pico W, display OLED SSD1306, buzzer com PWM e LEDs indicadores.

---

## Descrição

Este projeto implementa um servidor web simples no Raspberry Pi Pico W que permite controlar duas esteiras (motores) via navegador, acionando LEDs indicadores, exibindo mensagens no display OLED e emitindo sons no buzzer.

O dispositivo conecta-se à rede Wi-Fi especificada, cria um servidor HTTP na porta 80 e oferece botões para:

- Ligar Motor A
- Ligar Motor B
- Desligar ambas as esteiras

O display OLED exibe o status atual das esteiras, e o buzzer emite um beep ao ligar cada motor.

---

## Hardware Utilizado

- **Raspberry Pi Pico W**
- **Display OLED SSD1306** (I2C 128x64)
- **Buzzer piezo** conectado ao pino GPIO 10 (com PWM)
- LEDs indicadores:
  - LED vermelho no GPIO 13
  - LED azul no GPIO 12
- Botão B no GPIO 6 para resetar em modo BOOTSEL

---

## Pinos Utilizados

| Função               | GPIO     |
|----------------------|----------|
| LED vermelho         | 13       |
| LED azul             | 12       |
| Buzzer (PWM)         | 10       |
| I2C SDA para OLED    | 14       |
| I2C SCL para OLED    | 15       |
| Botão B (BOOTSEL)    | 6        |

---

## Como usar

1. Configure seu SSID e senha Wi-Fi no arquivo fonte (macros `WIFI_SSID` e `WIFI_PASSWORD`).
2. Compile e grave o firmware no Raspberry Pi Pico W.
3. Ligue o dispositivo e aguarde a conexão Wi-Fi.
4. No navegador, acesse o IP mostrado no terminal para abrir a página web de controle.
5. Use os botões para ligar o Motor A, Motor B ou desligar ambos.
6. Observe o status no display OLED e os LEDs.
7. O buzzer emitirá um beep ao ligar os motores, mas permanecerá silencioso ao desligar.

---

## Funcionalidades

- Inicialização Wi-Fi com timeout e reconexão.
- Servidor HTTP básico com páginas interativas.
- Controle de GPIOs para LEDs e buzzer.
- PWM para controle de frequência do buzzer.
- Display OLED SSD1306 para feedback visual.
- Modo BOOTSEL com botão B para resetar o dispositivo.

---

## Bibliotecas utilizadas

- `pico/stdlib.h`
- `hardware/adc.h`
- `pico/cyw43_arch.h`
- `hardware/pwm.h`
- `lwip` (TCP/IP stack para servidor web)
- SSD1306 OLED driver (`lib/ssd1306.h` e `lib/font.h`)

---

## Estrutura do código

- **main()**: inicializa periféricos, Wi-Fi e servidor TCP.
- **gpio_init_esteira()**: configura GPIOs dos LEDs e buzzer.
- **buzzer_init_pwm()** e **buzzer_beep()**: gerenciam PWM do buzzer.
- **display_msg()**: atualiza mensagem no OLED.
- **process_request()**: interpreta requisições HTTP e controla hardware.
- **tcp_server_accept()** e **tcp_server_recv()**: gerenciam conexões e dados HTTP.

---

## Observações

- Para entrar em modo BOOTSEL, pressione o botão B.
- Frequência e duração do beep do buzzer podem ser ajustadas na função `buzzer_beep`.
- O display OLED limpa e redesenha a moldura a cada atualização.
