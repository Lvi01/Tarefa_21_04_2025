// Projeto: GreenLife - Vaso Inteligente para Monitoramento de Plantas
// Autor: Levi Silva Freitas
// Data: 19/04/2025

// Descrição: Código para controle de um vaso inteligente com display OLED, LEDs RGB e buzzer
// O código lê os valores de temperatura e umidade de sensores analógicos, exibe os dados em um display OLED e controla LEDs RGB e um buzzer para feedback visual e sonoro.

// Bibliotecas necessárias para o funcionamento do código
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "inc/font.h"
#include "inc/ssd1306.h"

// Arquivo .pio
#include "./final.pio.h" // Garantir que o programa final esteja declarado

// I2C - Display OLED
#define I2C_ENDERECO 0x3C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// LEDs RGB
#define LED_VERMELHO 13
#define LED_AZUL 12
#define LED_VERDE 11

// Pino de saída
#define LED_MATRIX 7

// Número de LEDs
#define NUM_LEDS 25

// Buzzer
#define BUZZER 21
#define BUZZER_PWM_SLICE pwm_gpio_to_slice_num(BUZZER)
#define BUZZER_PWM_CHANNEL pwm_gpio_to_channel(BUZZER)

// Entradas analógicas simuladas pelo joystick
#define SENSOR_TEMPERATURA 27  // eixo X
#define SENSOR_UMIDADE 26      // eixo Y

// Botões para alternar o display
#define BOTAO_TEMP 5
#define BOTAO_UMID 6

// Variáveis globais
static volatile uint64_t debounce_antes = 0;
static volatile bool alarme_ativo = false;
static volatile bool exibir_temp = false;
static volatile bool exibir_umidade = false;
ssd1306_t display;

PIO pio = pio0;
uint sm;
uint32_t valor_led;

// Planta e vaso em matriz 5x5: 0 = apagado, 1 = planta, 2 = vaso
uint8_t planta_matriz[25] = {
    2, 2, 2, 2, 2,
    0, 0, 1, 0, 0,
    0, 0, 1, 1, 0,
    1, 1, 1, 1, 0,
    1, 1, 1, 0, 0
};

// Prototipação
bool inicializar_sistema();
static void irq_botoes(uint gpio, uint32_t eventos);
void configurar_pwm(uint gpio);
void atualizar_display_grafico(uint16_t temperatura, uint16_t umidade);
void atualizar_feedback_visual(uint16_t temperatura, uint16_t umidade);
void atualizar_display_dados(uint16_t temperatura, uint16_t umidade);
void acionar_alarme();
void animacao_matriz_leds(float temperatura);
uint32_t matrix_rgb(double b, double r, double g);
bool alarme_periodico_callback(repeating_timer_t *t);

int main() {
    stdio_init_all();
    if (!inicializar_sistema()) {
        printf("Erro na inicialização dos componentes.\n");
        return 1;
    }

    // Inicializa timer para relatórios periódicos
    static repeating_timer_t timer;
    add_repeating_timer_ms(5000, alarme_periodico_callback, NULL, &timer);

    while (true) {
        adc_select_input(1);
        uint16_t temperatura_adc = adc_read();

        adc_select_input(0);
        uint16_t umidade_adc = adc_read();

        atualizar_feedback_visual(temperatura_adc, umidade_adc);
        animacao_matriz_leds(temperatura_adc);

        if (exibir_temp || exibir_umidade) {
            atualizar_display_dados(temperatura_adc, umidade_adc);
        } else {
            atualizar_display_grafico(temperatura_adc, umidade_adc);
        }

        sleep_ms(200);

        const uint16_t frequencias[] = {1000, 1500};
        const uint16_t duracao = 300;
        uint8_t i = 0;

        while (alarme_ativo) {
            pwm_set_clkdiv(BUZZER_PWM_SLICE, 125.0f);
            pwm_set_wrap(BUZZER_PWM_SLICE, 125000 / frequencias[i]);
            pwm_set_chan_level(BUZZER_PWM_SLICE, BUZZER_PWM_CHANNEL, 125000 / frequencias[i] / 2);
            sleep_ms(duracao);
            i = !i;
        }
        pwm_set_chan_level(BUZZER_PWM_SLICE, BUZZER_PWM_CHANNEL, 0);
    }
    return 0;
}

bool inicializar_sistema() {
    adc_init();
    adc_gpio_init(SENSOR_TEMPERATURA);
    adc_gpio_init(SENSOR_UMIDADE);

    gpio_init(BOTAO_TEMP);
    gpio_set_dir(BOTAO_TEMP, GPIO_IN);
    gpio_pull_up(BOTAO_TEMP);
    gpio_set_irq_enabled_with_callback(BOTAO_TEMP, GPIO_IRQ_EDGE_FALL, true, &irq_botoes);

    gpio_init(BOTAO_UMID);
    gpio_set_dir(BOTAO_UMID, GPIO_IN);
    gpio_pull_up(BOTAO_UMID);
    gpio_set_irq_enabled_with_callback(BOTAO_UMID, GPIO_IRQ_EDGE_FALL, true, &irq_botoes);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, WIDTH, HEIGHT, false, I2C_ENDERECO, I2C_PORT);
    ssd1306_config(&display);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);

    configurar_pwm(LED_VERDE);
    configurar_pwm(LED_AZUL);
    configurar_pwm(LED_VERMELHO);
    configurar_pwm(BUZZER);

    // Inicia PIO para controlar matriz de LEDs
    extern const pio_program_t final_program;
    uint offset = pio_add_program(pio, &final_program);
    sm = pio_claim_unused_sm(pio, true);
    final_program_init(pio, sm, offset, LED_MATRIX);

    return true;
}

static void irq_botoes(uint gpio, uint32_t eventos) {
    uint64_t agora = to_us_since_boot(get_absolute_time());
    if ((agora - debounce_antes) < 200000)
        return;
    debounce_antes = agora;

    if (!alarme_ativo) {
        if (gpio == BOTAO_TEMP) {
            exibir_temp = !exibir_temp;
            exibir_umidade = false;
            printf("\nBotão TEMP pressionado\n");
        } else if (gpio == BOTAO_UMID) {
            exibir_umidade = !exibir_umidade;
            exibir_temp = false;
            printf("\nBotão UMIDADE pressionado\n");
        }
    } else {
        acionar_alarme();
    }
}

void configurar_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 4095);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0);
    pwm_set_enabled(slice, true);
}

void atualizar_feedback_visual(uint16_t temperatura, uint16_t umidade) {
    static uint64_t tempo_inicio_alerta = 0;
    uint64_t agora = to_us_since_boot(get_absolute_time());

    if (temperatura > 4000 || temperatura < 20 || umidade > 4000 || umidade < 20) {
        if (tempo_inicio_alerta == 0) tempo_inicio_alerta = agora;
        if ((agora - tempo_inicio_alerta) > 5000000) {
            printf("Alerta de condição crítica\n");
            acionar_alarme();
        }
        pwm_set_gpio_level(LED_VERDE, 0);
        pwm_set_gpio_level(LED_AZUL, 0);
        pwm_set_gpio_level(LED_VERMELHO, 4095);
        ssd1306_draw_string(&display, "PERIGO", 10, 30);
    } else {
        tempo_inicio_alerta = 0;
        if (temperatura > 3000 || temperatura < 1000 || umidade > 3000 || umidade < 1000) {
            pwm_set_gpio_level(LED_VERDE, 4095);
            pwm_set_gpio_level(LED_AZUL, 0);
            pwm_set_gpio_level(LED_VERMELHO, 4095);
        } else {
            pwm_set_gpio_level(LED_VERDE, 4095);
            pwm_set_gpio_level(LED_AZUL, 0);
            pwm_set_gpio_level(LED_VERMELHO, 0);
        }
    }
}

void atualizar_display_grafico(uint16_t temperatura, uint16_t umidade) {
    uint8_t x = (temperatura * (WIDTH - 8)) / 4095;
    uint8_t y = (umidade * (HEIGHT - 8)) / 4095;

    ssd1306_fill(&display, false);
    ssd1306_rect(&display, y, x, 8, 8, true, true);
    ssd1306_send_data(&display);
}

void atualizar_display_dados(uint16_t temperatura, uint16_t umidade) {
    char temp_str[6];
    char umidade_str[6];
    sprintf(temp_str, "%d", temperatura);
    sprintf(umidade_str, "%d", umidade);

    ssd1306_fill(&display, false);
    if (!alarme_ativo) {
        if (exibir_temp) {
            ssd1306_draw_string(&display, "Temp (ADC):", 3, 10);
            ssd1306_draw_string(&display, temp_str, 50, 20);
        } else if (exibir_umidade) {
            ssd1306_draw_string(&display, "Umid (ADC):", 3, 10);
            ssd1306_draw_string(&display, umidade_str, 50, 20);
        }
        ssd1306_send_data(&display);
    } else {
        ssd1306_draw_string(&display, "ALARME", 10, 10);
        ssd1306_draw_string(&display, "DISPARADO", 10, 20);
        ssd1306_draw_string(&display, "Aperte um", 10, 30);
        ssd1306_draw_string(&display, "botao p/", 10, 40);
        ssd1306_draw_string(&display, "desativar", 10, 50);
        ssd1306_send_data(&display);
    }
}

void acionar_alarme() {
    alarme_ativo = !alarme_ativo;
    if (alarme_ativo) {
        pwm_set_gpio_level(LED_VERDE, 0);
        pwm_set_gpio_level(LED_AZUL, 0);
        pwm_set_gpio_level(LED_VERMELHO, 4095);
        printf("Alarme ativado!\n");
    } else {
        pwm_set_gpio_level(BUZZER, 0);
        printf("Alarme desativado!\n");
    }
}

void animacao_matriz_leds(float temperatura) {
    double r = 0.0, g = 1.0, b = 0.0;

    // Converte temperatura em cor da planta: verde (ideal), vermelho (quente), azul (frio)
    if (temperatura > 3000) {
        r = 1.0; g = 0.0; b = 0.0; // vermelho
    } else if (temperatura < 50) {
        r = 0.0; g = 0.0; b = 1.0; // azul
    }

    for (int i = 0; i < NUM_LEDS; i++) {
        if (planta_matriz[i] == 1) {
            valor_led = matrix_rgb(b, r, g); // cor da planta
        } else if (planta_matriz[i] == 2) {
            valor_led = matrix_rgb(0.0, 1.0, 1.0); // amarelo do vaso
        } else {
            valor_led = matrix_rgb(0.0, 0.0, 0.0); // apagado
        }
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R = r * 255;
    unsigned char G = g * 255;
    unsigned char B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

bool alarme_periodico_callback(repeating_timer_t *t) {
    adc_select_input(0);
    float umidade = (adc_read() / 4095.0f) * 100.0f; // Simulação de temperatura em Celsius de 0 a 70 graus

    adc_select_input(1);
    float temperatura = (adc_read() / 4095.0f) * 70.0f; // Simulação de umidade em porcentagem de 0 a 100%

    printf("[Relatorio Periodico] Temperatura: %.2f C | Umidade: %.2f %%\n", temperatura, umidade);
    if(umidade > 80.0 || umidade < 20.0)
        printf("Dê atenção a sua planta!!\nUmidade fora do intervalo!\n");
    else if(temperatura > 50.0 || temperatura < 10.0)
        printf("Dê atenção a sua planta!!\nTemperatura fora do intervalo!\n");
    else
        printf("Sua planta está feliz!!\nTemperatura e umidade dentro do intervalo.\n");
    return true;
}