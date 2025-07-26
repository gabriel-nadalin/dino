# T-REX GAME EM FPGA
Uma implementação do clássico jogo T-Rex do Google Chrome para FPGA DE10-Standard com saída de vídeo VGA.

Este projeto foi desenvolvido como parte da disciplina Projeto de Sistemas Computacionais Embarcados, com o objetivo de aplicar conceitos de programação em baixo nível, interface com hardware e renderização gráfica em sistemas embarcados.  

- Professor Emerson Carlos Pedrino  
- Bacharelado em Ciência da Computação
- Departamento de Computação
- Universidade Federal de São Carlos

## Descrição
Este projeto recria o famoso jogo offline do Google Chrome (T-Rex Runner) em hardware utilizando uma placa FPGA DE10-Standard da Intel/Altera. O jogo é exibido através de uma saída VGA e controlado pelos botões físicos da FPGA, proporcionando uma experiência retro de gaming em hardware dedicado.

## Features
* Dinossauro jogável: Sprite animado com sistema de pulo realista
* Obstáculos: Cactos que se movem da direita para a esquerda
* Cenário dinâmico: Nuvens decorativas e chão texturizado
* Sistema de pontuação: Score incrementa conforme o progresso
* Dificuldade progressiva: Velocidade aumenta a cada 100 pontos
* Detecção de colisão: Sistema preciso de hitbox
* Game Over e reinício: Funcionalidade completa de reset

## Hardware utilizado
* DE10-Standard
## Software utilizado
* Quartus Lite 18.1
* Altera Monitor Program 18.1

## Como executar
* Clone o repositório
* Conecte a FPGA ao computador
* Conecte a saída VGA a um monitor
* Abra o Altera Monitor Program
* Crie um novo projeto usando o modelo "video" como base. Certifique-se de selecionar o _device_ correto
* Substitua o código dentro do `video.c` pelo conteúdo do `dino.c`
* Compile o código e carregue-o na FPGA

## Módulos
* Renderização na VGA: Funções responsáveis para desenhar sprites e manipular pixels
* Input: Detecção de interação com o usuário a partir dos botões da FPGA
* Jogo: Funções para atualizar o estado de jogo entre os frames e detectar colisões
* Geração procedural: Funções para geração do mapa

## Demonstração

https://github.com/user-attachments/assets/758e0d5b-0491-406a-b9c5-0e2ce750eec1
