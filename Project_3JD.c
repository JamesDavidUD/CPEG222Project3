/*===================================CPEG222====================================
 * Program:		Project_3JD.c
 * Authors: 	James David
 * Date: 		11/2/2020
 * Description: Project_3JD.c uses PMOD Keypad and the boards LEDs, SSDS, and LCD
 * to play a guessing game between 1-4 players
 * Input: PMOD Keypad
 * Output: LCD,LEDs, and the SSD are constantly updated to show progress of the game
==============================================================================*/
/*------------------ Board system settings. PLEASE DO NOT MODIFY THIS PART ----------*/
#ifndef _SUPPRESS_PLIB_WARNING          //suppress the plib warning during compiling
    #define _SUPPRESS_PLIB_WARNING      
#endif
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT             // Primary Oscillator Configuration (XT osc mode)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)

#pragma config FSRSSEL = PRIORITY_7
#pragma config JTAGEN = OFF
/*----------------------------------------------------------------------------*/
      
#include <xc.h>   //Microchip XC processor header which links to the PIC32MX370512L header
#include <p32xxxx.h>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <sys/attribs.h>
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"
#include "led.h"
#include "ssd.h"
#include "acl.h"

//Port JA
#define R4 LATCbits.LATC3
#define R3 LATGbits.LATG7 
#define R2 LATGbits.LATG8 
#define R1 LATGbits.LATG9 
#define C4 PORTCbits.RC2
#define C3 PORTCbits.RC1
#define C2 PORTCbits.RC4
#define C1 PORTGbits.RG6  

/* --------------------------- Forward Declarations-------------------------- */
void delay_ms(int ms); //function for delaying the code
void CNConfig();
void T2Config();
void GuessingGame(int number);
void SSDShowRange();

/* -------------------------- Definitions------------------------------------ */
#define SYS_FREQ    (80000000L) // 80MHz system clock
#define SECONDS_TIMEOUT 20 //How long a turn is (in seconds)
int press = 0;
int button=18; //value for Keypad to write  
int num_players; //total number of players
char str[20]; //empty string
int num_to_guess; //number to guess during the game
int current_player = 1; //current player that is guessing
int low_range =0, high_range=99; //range
enum gameMode{Player_Config, Config_Secret, Guessing}mode; //An enum to make each mode a word and not a number 
                                                           //so the code is easier to read and write
int led_array[5] = {0x0,0x1,0x2,0x4,0x8}; //array to store LED positions
int updated = 1; //helps with flicker on the SSD
int seconds_passed = 0; //counts how mahy seconds have passed
int guess1=18,guess2=18; //digits of the current guess, 1 is the ones digit, 2 is the tens digit
int sec1=SECONDS_TIMEOUT%10,sec2=(SECONDS_TIMEOUT/10)%10; //Time left in digit form, sec 1 is ones digit, 2 is tens digit
int stop = 0,stop2 = 0, inSSD = 0; //flags to help with flicker
/* ----------------------------- Main --------------------------------------- */
int main(void) 
{
 /*-------------------- Port and State Initialization -------------------------*/
    DDPCONbits.JTAGEN = 0; 
    LED_Init();                 // Initialize the LEDs
    LCD_Init();                 // Initialize the LCD
    SSD_Init();                 // Initialize the SSD
    
    //setting seed using accelerometer
    ACL_Init();    
    float rgACLGVals[3];    
    ACL_ReadGValues(rgACLGVals);
    int seed = rgACLGVals[0] * 10000;    
    srand((unsigned) seed);
    
    
    //Port JA
    //rows as outputs
    TRISGbits.TRISG7 = 0; 
    TRISGbits.TRISG8 = 0; 
    TRISCbits.TRISC3 = 0;
    TRISGbits.TRISG9 = 0;
    
    ANSELGbits.ANSG7 = 0;
    ANSELGbits.ANSG8 = 0;
    ANSELGbits.ANSG9 = 0;
    
    //columns as inputs
    TRISGbits.TRISG6 = 1;
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC4 = 1;
    TRISCbits.TRISC1 = 1;
    
    ANSELGbits.ANSG6 = 0;
      
    R1 = 0; 
    R2 = 0;
    R3 = 0;
    R4 = 0;
    
    //configuring Timer2 and Keypad
    CNConfig();
    T2Config();
    //Clearing up mismatches
    int t = PORTG;
    int j = PORTC;
/*-------------------- Main logic and actions start --------------------------*/
    
    mode = Player_Config; //Starts the Game
    while (1)
    { 
        switch(mode){
            case Player_Config:
                LED_SetGroupValue(0);
                SSD_WriteDigits(18,18,18,18,0,0,0,0);
                if(button==18|| button>4 || button<1){ //reads only 1-4 as input
                LCD_WriteStringAtPos("Num Guessing    ",0,0);
                LCD_WriteStringAtPos("# Players? (1-4)",1,0);
                }
                else{
                    num_players = button;
                    sprintf(str, "Num Guessing - %d", num_players);
                    LCD_WriteStringAtPos(str, 0, 0);
                    LCD_WriteStringAtPos("                ",1,0);
                    mode = Config_Secret;
                }
                break;
            case Config_Secret:
                LCD_WriteStringAtPos(str, 0, 0);
                LCD_WriteStringAtPos("Rand(E), Det(D)?",1,0);
                if(button==13){
                    //If D was pressed
                    num_to_guess = 69; //Hard-Coded Number
                    LCD_WriteStringAtPos("Deterministic   ",1,0);
                    delay_ms(500);
                    mode = Guessing;
                }
                if(button==14){
                    //If E was pressed
                    num_to_guess = rand()%100; //random number between 0 and 99
                    LCD_WriteStringAtPos("Random secret   ",1,0);
                    delay_ms(500);
                    mode = Guessing;  
                }
                button = 18;
                break;
            case Guessing:
                current_player=1;
                while(current_player <= num_players){ //loops through every player
                    GuessingGame(current_player);
                    current_player++;
                }  
                break;
            
        }
       
    }
/*--------------------- Action and logic end ---------------------------------*/
}
/* ----------------------------------------------------------------------------- 
**	delay_ms
**	Parameters:
**		ms - amount of milliseconds to delay (based on 80 MHz SSCLK)
**	Return Value:
**		none
**	Description:
**		Create a delay by counting up to counter variable
** -------------------------------------------------------------------------- */
void delay_ms(int ms){
	int		i,counter;
	for (counter=0; counter<ms; counter++){
        for(i=0;i<1426;i++){}   //software delay 1 millisec
    }
}

void GuessingGame(int guesser){
    char str2[20];
    int guessed_num = 0;
    int guess_inputed = 0;
    stop = 0;
    guess1 = 18;
    guess2 = 18;
    sprintf(str2, "Player %d Guess  ", guesser);  
    LCD_WriteStringAtPos(str2, 0, 0);
    LCD_WriteStringAtPos("                ",1,0);
    LED_SetGroupValue(led_array[guesser]); //lights up LED 0-3 depending on which player is up
    sec1 = (SECONDS_TIMEOUT - seconds_passed)%10; //starting digits for the timer
    sec2 = ((SECONDS_TIMEOUT - seconds_passed)/10)%10;
    SSD_WriteDigits(guess1,guess2,sec1,sec2,0,0,0,0); //writes Starting timer to SSD
    while(guess_inputed==0){  
        if(button!=18){ //if any button is pressed
            if(button>=0 && button<10){ //if any number button was pressed
                guess2 = guess1;
                guess1 = button;
                updated = 1;
            }
            if(button==12){ //If C was pressed, clears all guess digits
                guess2 = 18;
                guess1 = 0;
                updated = 1;
            }
            if(button==13){ //If D was pressed, deletes ones digit of current num
                guess1=guess2;
                guess2=18;
                updated = 1;
            } 
            if(button==14){ //If E was pressed
                stop=1;
                guess_inputed=1;
                if(guess1==18&&guess2==18){ //if no number was inputed
                        guess_inputed=0;
                        stop=0;
                    }
                else{
                    if(guess2==18){guess2=0;}
                    if(guess1==18){guess1=0;} 
                }
               
                guessed_num = guess2*10 + guess1; //combined guess, actual value of what the player is guessing
                if(guessed_num<num_to_guess){
                    if(guessed_num<low_range){ //if number guess is too low, but out of range
                        LCD_WriteStringAtPos("Out of range    ",1,0);
                        guess_inputed=0;
                        stop=0;
                    }
                    else{//if number guess is too low, and in range
                        LCD_WriteStringAtPos("Too Low!         ",1,0);
                        low_range = guessed_num + 1;
                    }
                }
                if(guessed_num>num_to_guess){//if number guess is too high,
                    if(guessed_num>high_range){//if num was out of range
                        if(guess1==18&&guess2==18){ //if number guess is too low, but out of range
                            LCD_WriteStringAtPos("Enter A Number          ",1,0); //a case for when a player enters nothing
                        }
                        else{
                            LCD_WriteStringAtPos("Out of range    ",1,0);
                        }
                        guess_inputed=0;
                        stop=0;
                    }
                    else{//if number was in range
                        LCD_WriteStringAtPos("Too High!        ",1,0);
                        high_range = guessed_num - 1;
                    }
                }
                if(guessed_num==num_to_guess){ //if guessed number was correct
                    button=18;
                    LCD_WriteStringAtPos("You Got It!          ",1,0);
                    delay_ms(1000);
                    SSD_WriteDigits(guess1,guess2,sec1,sec2,0,0,0,0);
                    while(button==18){ //loops until the player presses any key
                        LCD_WriteStringAtPos("Press Any Key            ",0,0);
                    }
                    //resets game values after player wins
                    current_player=99;
                    low_range = 0;
                    high_range = 99;
                    button = 18;
                    mode = Player_Config;
                    break;
                }
                button=18;
                delay_ms(1000);
                updated=1;
            }
            if(button==15){ //If F was pressed, shows range on SSD
                button=18;
                inSSD=1;
                while(seconds_passed<SECONDS_TIMEOUT){ //will cut off showing range if players turn is done
                    SSDShowRange();
                    if(button==15){
                        break;
                    }
                }
                SSD_WriteDigits(guess1,guess2,sec1,sec2,0,0,0,0);
                LCD_WriteStringAtPos("                  ",1,0);
                //reseting values
                inSSD=0;
                stop = 0;
                stop2 = 0;
            }
            if(updated==1){//writes to ssd only if a value has changed, prevents flicker
                SSD_WriteDigits(guess1,guess2,sec1,sec2,0,0,0,0);
                updated=0;
            }
            button=18;
        }
        if(seconds_passed==20){
            guess_inputed=1;
        }
    }
    seconds_passed=0;
    guess_inputed=0;
}
void SSDShowRange(){ //shows current range
    int low1 = low_range%10;
    int low2 = (low_range/10)%10;
    int high1= high_range%10;
    int high2= (high_range/10)%10;
    LCD_WriteStringAtPos("Showing Range        ",1,0);
    if(stop2==0){ //makes it so the range is only displayed once, prevents flicker
        SSD_WriteDigits(high1,high2,low1,low2,0,0,0,0);
    }
    stop2=1;
}
void __ISR(_CHANGE_NOTICE_VECTOR) keyPad(void){
    int i;
    CNCONCbits.ON = 0;
    CNCONGbits.ON = 0;
    for (i=0; i<1000; i++); //de-bouncing
    
    unsigned char keyCurrState = (!C1 ||!C2 ||!C3 ||!C4); //a button lock, prevents more than one input being recognized
    
    if(!keyCurrState){
        press = 0;
    }
    else if(!press){
        press = 1;
        R1 = 0;
        R2 = 1;
        R3 = 1;
        R4 = 1;
        
        if(C1==0){
            //If button pressed was 1
            button = 1;
        } else if(C2==0){
            //If button pressed was 2
            button = 2;
        } else if(C3==0){
            //If button pressed was 3
            button = 3;
        } else if(C4==0){
            //If button pressed was 'A'
            button = 10;
        }
        
        R1 = 1;
        R2 = 0;
        if(C1==0){
            //If button pressed was 4
            button = 4;
        } else if(C2==0){
            //If button pressed was 5
            button = 5;
        } else if(C3==0){
            //If button pressed was 6
            button = 6;
        } else if(C4==0){
            //If button pressed was 'B'
            button = 11;
        }
        
        R2 = 1;
        R3 = 0;
        if(C1==0){
            //If button pressed was 7
            button = 7;
        } else if(C2==0){
            //If button pressed was 8
            button = 8;
        } else if(C3==0){
            //If button pressed was 9
            button = 9;
        } else if(C4==0){
            //If button pressed was 'C'
            button = 12;
        }
        
        R3 = 1;
        R4 = 0;
        if(C1==0){
            //If button pressed was 0
            button = 0;
        } else if(C2==0){
            //If button pressed was 'F'
            button = 15;
        } else if(C3==0){
            //If button pressed was 'E'
            button = 14;
        } else if(C4==0){
            //If button pressed was 'D'
            button = 13;
        }
        
        //done with checking if one key was pressed
        //reset
        R1 = 0;
        R2 = 0;
        R3 = 0;
        R4 = 0;
        
    }
    
    int j = PORTC;
    int u = PORTG;
    
    IFS1bits.CNCIF = 0;
    IFS1bits.CNGIF = 0;
    
    CNCONCbits.ON = 1;
    CNCONGbits.ON = 1;
    
}
void CNConfig(){
    macro_disable_interrupts;
    //CNCONDbits.ON = 1;
    CNCONCbits.ON = 1;
    CNCONGbits.ON = 1;
   
    CNENC = 0x16;
    CNPUC = 0x16;
    
    CNENG = 0x40;
    CNPUG = 0x40;
     
   
    IPC8bits.CNIP = 6;
    IPC8bits.CNIS = 3;
    
    IFS1bits.CNCIF = 0;
    IEC1bits.CNCIE = 1;
    
    IFS1bits.CNGIF = 0;
    IEC1bits.CNGIE = 1;
  
    macro_enable_interrupts();
    
}
void T2Config(){
    PR2 = 39062; //1*10^7 (or 10 MHz)/256 - 1 and rounded up
    TMR2 = 0;
    
    T2CONbits.ON = 0;
    T2CONbits.TCS = 0;
    T2CONbits.TGATE = 0;
    T2CONbits.TCKPS = 7;
    
    IPC2bits.T2IP = 7;
    IPC2bits.T2IS = 4;
    IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 1;
    T2CONbits.ON = 1;
    macro_enable_interrupts(); 
}

void __ISR(_TIMER_2_VECTOR, IPL7SOFT) Timer2ISR(void){
    if((mode==Guessing)&&stop==0){ //will update timer only during Guessing
        seconds_passed++;
        sec1 = (SECONDS_TIMEOUT - seconds_passed)%10;
        sec2 = ((SECONDS_TIMEOUT - seconds_passed)/10)%10;
        if(inSSD==0){ //Prevents timer from messing with ShowRange
            SSD_WriteDigits(guess1,guess2,sec1,sec2,0,0,0,0);
        }
    }
    IFS0bits.T2IF=0; 
}