#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lib/ssd1306.h>
#include <lib/font.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define WIFI_SSID "Gama"
#define WIFI_PASSWORD "pati2081"


// ====== Definições de pinos ======
#define LED_RED_PIN 13
#define LED_BLUE_PIN 12
#define BUZZER_PIN 10

// ====== I2C para o OLED ======
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

//Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

//Funcao para inicializar o PWM no Buzzer
void buzzer_init_pwm(void) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 12500);  // 125 MHz / 12500 = 10 kHz (ajustável)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 0);  // Começa em 0 (sem som)
    pwm_set_enabled(slice_num, true);
}

//Função para tocar o buzzer
void buzzer_beep(uint freq, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint clk_div = 125000000 / (freq * 100); // 100 steps no ciclo (ajustável)
    if (clk_div < 1) clk_div = 1;
    if (clk_div > 255) clk_div = 255;

    pwm_set_clkdiv(slice_num, clk_div);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 50); // 50% duty

    sleep_ms(duration_ms);

    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 0); // Silenciar
}


// ====== Objeto do display ======
ssd1306_t ssd;
bool cor = true;

// ====== Protótipos ======
void gpio_init_esteira(void);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void process_request(char **request);
void display_msg(const char *msg);

// ====== Função para exibir mensagem no display ======
void display_msg(const char *msg) {
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Moldura
    ssd1306_draw_string(&ssd, msg, 10, 28); // Mensagem centralizada
    ssd1306_send_data(&ssd); // Atualiza o display
}

// ====== Função principal ======
int main() {
    stdio_init_all();
    //Trecho do botao bootsel
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    //Encerramento desse trecho
    
    gpio_init_esteira();
    buzzer_init_pwm(); 

    // Inicializa I2C para o display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    display_msg("Aguardando comando");

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar\n");
        sleep_ms(1000);
    }
    printf("Conectado! IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));

    struct tcp_pcb *server = tcp_new();
    if (!server || tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao criar servidor\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("Servidor web iniciado na porta 80\n");

    while (true) {
        cyw43_arch_poll();
        sleep_ms(100);
    }

    cyw43_arch_deinit();
    return 0;
}

// ====== Inicializa GPIOs ======
void gpio_init_esteira(void) {
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0);

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, 0);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

// ====== Callback de conexão ======
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// ====== Processa a solicitação HTTP ======
void process_request(char **request) {
    if (strstr(*request, "GET /motor_a") != NULL) {
        gpio_put(LED_RED_PIN, 1);
        gpio_put(LED_BLUE_PIN, 0);
         buzzer_beep(1000, 200); 
        display_msg("Esteira A ON");
    } else if (strstr(*request, "GET /motor_b") != NULL) {
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_BLUE_PIN, 1);
        buzzer_beep(200, 200);
        display_msg("Esteira B ON");
    } else if (strstr(*request, "GET /desligar") != NULL) {
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_BLUE_PIN, 0);
        //gpio_put(BUZZER_PIN, 0);
        display_msg("Esteiras desligadas");
    }
}

// ====== Recebe dados HTTP ======
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);
    process_request(&request);

    char html[1024];
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Esteira Inteligente</title>\n"
        "<style>\n"
        "body { font-family: Arial; background: #eef; text-align: center; margin-top: 50px; }\n"
        "button { font-size: 30px; padding: 20px; margin: 10px; border-radius: 10px; }\n"
        "</style>\n"
        "</head><body>\n"
        "<h1>Controle da Esteira</h1>\n"
        "<form action=\"/motor_a\"><button>Ligar motor A</button></form>\n"
        "<form action=\"/motor_b\"><button>Ligar motor B</button></form>\n"
        "<form action=\"/desligar\"><button>Desligar as Maquinas</button></form>\n"
        "</body></html>");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    free(request);
    pbuf_free(p);
    return ERR_OK;
}