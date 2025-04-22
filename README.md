# GreenLife – Monitoramento Ambiental com Vaso Inteligente

## Descrição  
O GreenLife é um sistema embarcado desenvolvido para simular e monitorar as condições ambientais de uma planta por meio de um vaso inteligente. Utilizando o kit BitDogLab e a placa Raspberry Pi Pico W, o projeto emprega sensores simulados com joystick, LEDs, display OLED e buzzer para apresentar o estado da planta em tempo real, oferecendo alertas visuais e sonoros baseados nos níveis de temperatura e umidade do solo.

## Objetivo  
O objetivo do projeto é simular condições reais de temperatura e umidade, proporcionando ao usuário uma visualização clara da “saúde” da planta. Além disso, busca servir como base para projetos futuros de automação residencial e agricultura urbana sustentável.

## Tecnologias e Hardware Utilizados  
- **Placa Microcontroladora**: Raspberry Pi Pico W  
- **Kit de Desenvolvimento**: BitDogLab  
- **Protocolos de Comunicação**: UART (monitoramento via terminal serial), I2C (comunicação com o display OLED)  
- **Sensores Utilizados**: Joystick (simulando temperatura e umidade)  
- **Atuadores**: LED RGB, matriz de LEDs 5x5 e buzzer para alertas visuais e sonoros  
- **Interface com o Usuário**: Display OLED para exibição gráfica e numérica, botões físicos para interação

## Estrutura do Projeto  
1. **Aquisição de Dados**: As entradas analógicas do joystick simulam as medições de temperatura (eixo X) e umidade (eixo Y).  
2. **Processamento e Lógica**: Os dados são interpretados e classificados em três faixas (ideal, atenção e crítica), acionando os periféricos conforme o estado.  
3. **Atuação e Alertas**: Em situações de risco, o sistema aciona o buzzer e altera as cores dos LEDs RGB e da matriz de LEDs para alertar o usuário.  
4. **Exibição de Dados**: O display OLED alterna entre modos gráficos e numéricos, exibindo as informações em tempo real.  
5. **Interação com o Usuário**: Dois botões físicos permitem alternar modos de exibição e silenciar alarmes.  
6. **Comunicação Serial**: As informações também são transmitidas via UART para um terminal serial, permitindo o monitoramento remoto.  
7. **Controle de Eventos**: Interrupções são utilizadas para os botões e para o temporizador, com tratamento de debounce por software para garantir estabilidade e precisão na leitura dos comandos.

## Como Executar o Projeto  
1. Conecte a placa Raspberry Pi Pico W ao computador.  
2. Abra a IDE apropriada (Thonny, VSCode com Pico SDK, etc.) para programação da placa.  
3. Compile e carregue o código-fonte no dispositivo.  
4. Utilize o joystick para simular as medições. Observe os resultados no display OLED, na matriz de LEDs e no terminal serial.  
5. Interaja com os botões para alternar modos ou silenciar alarmes quando o buzzer for ativado.

## Link do Projeto  
- **Demonstração no YouTube**: XXXXXXXX
## Autor  
Desenvolvido por Levi Silva Freitas.
