#include <stdio.h>
#include "conf.h"
#include "lcd.h"
#include "display7s.h"

#define TRUE 1
#define FALSE 0

#define PRESSIONADO 0

#define botaoLigaDes PORTBbits.RB0
#define botaoSensorSP PORTBbits.RB1
#define botaoSensorSS PORTBbits.RB2
#define botaoSensorSI PORTBbits.RB3

#define ledLigado PORTBbits.RB7
#define ledSensorSP PORTBbits.RB6
#define ledSensorSI PORTBbits.RB5
#define ledSensorSS PORTBbits.RB4

#define buzzer PORTCbits.RC1
#define heater PORTCbits.RC5
#define cooler PORTCbits.RC2

int statusTimer = FALSE; //Sensores e botão que trocam de estado como um switch
int desligado, enchendoDeAguaESabao, cicloDeLavagem, drenarAgua;
int liquidoSecante, escoamentoDoLiquidoSecante; //variáveis de monitoramento de estados
char estadoMes[15]; // String usada para demostrar o estado no LCD atráves da função

void tocaBuzzer(void) {
    for (int i = 0; i < 5; i++) {
        buzzer = TRUE;
        __delay_ms(10);
        buzzer = FALSE;
        __delay_ms(10);
    }
}

int timer(int min) { // Funcão timer decrecente, pode escolher o tempo em minutos pelo seu parametro min(quantidade de minutos)

    unsigned char segU, segD, minU, minD; //Usado para separar Minutos e Segundos em dezena e unidade

    TRISA = 0x00; //Configurações
    TRISD = 0x00;
    ADCON1 = 0x0F;

    for (unsigned char k = min; k > 0; k--) {

        minU = (k - 1) % 10; //Obter unidade da minuto
        minD = (k - 1) / 10; //Obter dezena da minuto
        for (unsigned char i = 60; i > 0; i--) {
            segU = (i - 1) % 10; //Obter unidade do segundo
            segD = (i - 1) / 10; //Obter dezena do segundo
            for (unsigned char j = 0; j < 30; j++) {
                PORTA = 0x20; // Disp4
                PORTD = display7s(segU); //Armazena o valor no display por meio da função de conversão
                __delay_ms(1);
                PORTA = 0x10; // Disp3
                PORTD = display7s(segD); //Armazena o valor no display por meio da função de conversão
                __delay_ms(1);
                PORTA = 0x08; // Disp2
                PORTD = display7s(minU); //Armazena o valor no display por meio da função de conversão
                __delay_ms(1);
                PORTA = 0x04; // Disp1
                PORTD = display7s(minD); //Armazena o valor no display por meio da função de conversão
                __delay_ms(1);
            }
        }
    }
    PORTA = 0; // Limpa os Displays
    return TRUE; // retorna TRUE para utilizamos como status do timer
}

void escreve(void)//função para demostrar o estado da máquina no display
{
    lcd_cmd(L_CLR); // Limpa o display
    lcd_cmd(L_L1 + 5); //Seleciona a primeira linha do display
    lcd_str("Estado: "); //demonstra "Estado :" no display
    lcd_cmd(L_L3 + 5);
    lcd_str(estadoMes); //demonstra o estado da maquina no display
}

void estadosMaquina(void) { // Função que verifica e troca os estados da máquina

    if ((desligado == TRUE) && (botaoLigaDes == PRESSIONADO)) {//ESTADO DESLIGADO, VERIFICA SE O BOTÃO DE LIGAR FOI PRESSIONADO
        desligado = FALSE; //LIGA A MAQUINA PARA O PROXIMO ESTADO
        ledLigado = TRUE;
        sprintf(estadoMes, "LIGADA");
        escreve();
        __delay_ms(20);
    } else if ((desligado == FALSE) && (botaoSensorSP == PRESSIONADO)) {//VERIFICA SE A PORTA ESTÁ FECHADA
        enchendoDeAguaESabao = TRUE; //ESTADO ENCHENDO DE ÁGUA E SABÃO
        ledSensorSP = TRUE;
        ledSensorSI = TRUE;
        sprintf(estadoMes, "ENCHENDO");
        heater = TRUE;
        escreve();
        __delay_ms(20);
    } else if (enchendoDeAguaESabao == TRUE) {//VERIFICA SE O EM QUAL ESTADO ESTÁ E SE HÁ ÁGUA NO SENSOR INFERIOR DA MÁQUINA
        if ((botaoLigaDes == PRESSIONADO) || (botaoSensorSP == PRESSIONADO)) {//VERIFICA SE A MÁQUINA FOI DESLIGADA OU TEVE SUA PORTA ABERTA
            heater = FALSE;
            enchendoDeAguaESabao = FALSE; //SAI DO ESTADO ANTERIOR
            escoamentoDoLiquidoSecante = TRUE; //VAI PARA O ESTADO DE ESCOAMENTO ANTES DE DESLIGAR A MÁQUINA
            ledSensorSP = FALSE;
            ledSensorSI = TRUE;
            sprintf(estadoMes, "ESCOANDO");
            escreve();
        } else if (botaoSensorSS == PRESSIONADO) {//VERIFICA SE ENCHEU A MAQUINA ATÉ O SENSOR SUPERIOR DETECTAR ÁGUA
            heater = FALSE;
            cooler = TRUE;
            enchendoDeAguaESabao = FALSE; //SAI DO ESTADO ANTERIOR
            ledSensorSS = TRUE;
            sprintf(estadoMes, "LAVANDO");
            cicloDeLavagem = TRUE; //VAI PARA O PRÓXIMO ESTADO
            escreve();
            __delay_ms(200);
            statusTimer = timer(20); //INICIA O TIMER DE 20 MIN DE LAVAGEM
            cooler = FALSE;
        }
    } else if (cicloDeLavagem == TRUE) {//VERIFICA EM QUAL ESTADO ESTÁ
        if ((botaoLigaDes == PRESSIONADO) || (botaoSensorSP == PRESSIONADO)) {//VERIFICA SE A MÁQUINA FOI DESLIGADA OU TEVE SUA PORTA ABERTA
            cicloDeLavagem = FALSE; //SAI DO ESTADO ANTERIOR
            escoamentoDoLiquidoSecante = TRUE; //VAI PARA O ESTADO DE ESCOAMENTO ANTES DE DESLIGAR A MÁQUINA
            ledSensorSI = TRUE;
            ledSensorSP = FALSE;
            sprintf(estadoMes, "ESCOANDO");
            escreve();
        } else if (statusTimer == TRUE) {//VERIFICA SE O TIMER FOI FINALIZADO
            cicloDeLavagem = FALSE; //SAI DO ESTADO ANTERIOR
            drenarAgua = TRUE; //VAI PARA O PROXIMO ESTADO
            ledSensorSS = FALSE;
            sprintf(estadoMes, "DRENANDO");
            escreve();
            statusTimer = FALSE; // VOLTA O STATUS DO TIMER PARA FALSE(TIMER NÃO FINALIZADO)
        }
    } else if (drenarAgua == TRUE) {//VERIFICA EM QUAL ESTADO ESTÁ
        if ((botaoSensorSS == PRESSIONADO) && (botaoSensorSI == PRESSIONADO)) {//VERIFICA SE OS SENSORES(INFERIOR E SUPERIOR) PARARAM DE DETECTAR ÁGUA
            drenarAgua = FALSE; //SAI DO ESTADO ANTERIOR
            liquidoSecante = TRUE; //VAI PARA O PRÓXIMO ESTADO
            ledSensorSP = FALSE;
            ledSensorSI = FALSE;
            sprintf(estadoMes, "LIQUIDO");
            escreve();
            statusTimer = timer(2); //INICIA O TIMER DE 2 MIN DE DISPERSÃO DO LÍQUIDO SECANTE
        } else if ((botaoSensorSP == PRESSIONADO) || (botaoLigaDes == PRESSIONADO)) { //VERIFICA SE A MÁQUINA FOI DESLIGADA OU TEVE SUA PORTA ABERTA
            drenarAgua = FALSE; // SAI DO ESTADO ANTERIOR
            escoamentoDoLiquidoSecante = TRUE; //VAI PARA O ESTADO DE ESCOAMENTO ANTES DE DESLIGAR A MÁQUINA
            ledSensorSP = FALSE;
            ledSensorSI = TRUE;
            sprintf(estadoMes, "ESCOANDO");
            escreve();
        }
    } else if ((liquidoSecante == TRUE) && 
       (botaoSensorSP == PRESSIONADO || botaoLigaDes == PRESSIONADO || statusTimer == TRUE)) {//VERIFICA O ESTADO ATUAL
        statusTimer = FALSE; //VOLTA O STATUS DO TIMER PARA NÃO CONCLUÍDO                                                        //E SE OU A MÁQUINA TEVE SUA PORTA ABERTA
        liquidoSecante = FALSE; //SAI DO ESTADO ANTERIOR                                                                      //OU FOI DESLIGADA OU O TIMER FOI CONCLUÍDO
        escoamentoDoLiquidoSecante = TRUE; //VAI PARA O PRÓXIMO ESTADO
        ledSensorSP = FALSE;
        ledSensorSI = TRUE;
        sprintf(estadoMes, "ESCOANDO");
        escreve();
    } else if (escoamentoDoLiquidoSecante == TRUE) {//VERIFICA EM QUE ESTADO ESTÁ
        if ((botaoSensorSI == PRESSIONADO)) {//VERIFICA SE O SENSOR INFERIOR PAROU DE DETECTAR ÁGUA
            escoamentoDoLiquidoSecante = FALSE; //SAI DO ESTADO ANTERIOR
            desligado = TRUE; //VAI PARA O PRÓXIMO ESTADO
            ledLigado = FALSE;
            ledSensorSI = FALSE;
            tocaBuzzer();
            ledSensorSS = FALSE;
            sprintf(estadoMes, "DESLIGADA");
            escreve();
        }
    }
}

void main(void) {
    TRISD = 0x00; // Portas D e E sao usadas no LCD
    TRISE = 0x00;
    TRISC = 0x00; //Configurações para usar o Cooler e heater
    TRISB = 0x0f;
    
    lcd_init(); //Configuração do lcd


    desligado = TRUE; // maquina inicia desligada
    enchendoDeAguaESabao = FALSE; //os outros estados estão aguardando para a transição de estados
    cicloDeLavagem = FALSE;
    drenarAgua = FALSE;
    liquidoSecante = FALSE;
    escoamentoDoLiquidoSecante = FALSE;

    ledLigado = FALSE;
    ledSensorSI = FALSE;
    ledSensorSS = FALSE;
    ledSensorSP = FALSE;
    while (1) { //Loop para verificação de estados da máquina

        estadosMaquina();
    }

}