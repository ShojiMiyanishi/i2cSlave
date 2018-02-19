/*
 * main.cpp
 *
 *  Created on: 2017/09/13
 *      Author: omiya
 */


#include <stdio.h>
#include <ctype.h>
#include "mbed.h"
#include "AsyncSerial.h"
const int indexI2C=0;
const int RX1=1;
const int RX2=2;
const int PC=3;
const int ESP=4;
DigitalOut led(LED1);
I2CSlave* slave;//(I2C_SDA, I2C_SCL);
AsyncSerial *uartPc;
AsyncSerial *uartEsp;
AsyncSerial *uartRx1;
AsyncSerial *uartRx2;
bool switchRx=true;
bool toggleSwitchRx(){
	if(switchRx){
		uartRx1->stopRx();
		uartRx2->stopRx();

	}else{
		uartRx1->startRx();
		uartRx2->startRx();

	}
	switchRx = !switchRx;
	return switchRx;

}
int main() {
   char buf[5][300];
   char msg[] = "Slave!";

   int len;
   char* ptrRx1=buf[RX1];
   char* ptrRx2=buf[RX2];
   char* ptrPc=buf[PC];
   char* ptcEsp=buf[ESP];
   int n1,n2;//文字数

   uartPc = new AsyncSerial(USBTX,USBRX);//PA_2,PA_3:USART2
   uartEsp = new AsyncSerial(PC_10,PC_11);//USART3
   uartEsp->baud(115200);
   uartRx1 = new AsyncSerial(PA_0,PA_1);//UART4
   uartRx2 = new AsyncSerial(PC_12,PD_2);//UART5
   uartRx1->baud(115200);
   uartRx2->baud(115200);
   uartPc->printf("I2C Slave\n\r");
   //uartEsp->printf("AT+RST\n");

   //DigitalIn* pin1,* pin2;
   //pin1 = new DigitalIn(I2C_SDA);
   //pin2= new DigitalIn(I2C_SCL);
   //pin1->mode(OpenDrainPullUp);
   //pin2->mode(PullUp);
   slave = new I2CSlave(I2C_SDA,I2C_SCL);
   slave->frequency(1000*1000);

   n1=n2=0;

   slave->address(0xA0);
   while (1) {
       int i = slave->receive();
       led=!led;
       switch (i) {
           case I2CSlave::ReadAddressed:
               //slave->write(msg, strlen(msg) + 1); // Includes null char
               break;
           case I2CSlave::WriteGeneral:
               slave->read(buf[indexI2C], sizeof(buf[indexI2C]));
               //printf("Read G: %s\n", buf);
               break;
           case I2CSlave::WriteAddressed:
               slave->read(buf[indexI2C], sizeof(buf[indexI2C]));
               len=uartPc->printf("%s", buf[indexI2C]);
               memset(buf[indexI2C],0,len);
//               for(int i = 0; i < sizeof(buf); i++) buf[i] = 0;    // Clear buffer
               break;
       }
       //len=uartRx1->readable();
       //if(len>0)
       {
    	   char *p=ptrRx1;
    	   char c;
    	   while(uartRx1->getc(c)){
			   if(c=='\r'){
				   n1++;
				   *p++=c;
				   *p++='\n';
				   *p++='\0';
				   uartPc->printf("[RX1]%4d:%s",n1,buf[RX1]);
				   p=buf[RX1];
				   n1=0;
				   break;
    		   }else
			   if(isprint(c)){
    			   if(c==' ')c='_';
				   *p++=c;
				   n1++;
			   }else{
				   n1++;
				   *p++='\\';
				   uint8_t a,b;
				   a=(c&0xf0)>>4;
				   b=c&0x0f;
				   *p++=(a>=10)?'A'+a-10:'0'+a;
				   *p++=(b>=10)?'A'+b-10:'0'+b;
			   }
			   if(p-buf[RX1]>sizeof(buf[RX1])-4){
				   *p++='\n';
				   *p++='\r';
				   *p++='\0';
				   uartPc->printf("[RX1]%4d:%s",n1,buf[RX1]);
				   p=buf[RX1];
				   n1=0;
			   }
    	   }
    	   ptrRx1=p;
       }
       //len=uartRx2->readable();
       //if(len>0)
       {
    	   char *p=ptrRx2;
    	   char c;
    	   while(uartRx2->getc(c)){
			   if(c=='\r'){
				   n2++;
				   *p++=c;
				   *p++='\n';
				   *p++='\0';
				   uartPc->printf("[RX2]%4d:%s",n2,buf[RX2]);
				   p=buf[RX2];
				   n2=0;
				   break;
    		   }else
    		   if(isprint(c)){
    			   n2++;
    			   if(c==' ')c='_';
    			   *p=c;
    		   }else{
    			   n2++;
    			   *p++='\\';
    			   uint8_t a,b;
				   a=(c&0xf0)>>4;
    			   b=c&0x0f;
    			   *p++=(a>=10)?'A'+a-10:'0'+a;
				   *p++=(b>=10)?'A'+b-10:'0'+b;
			   }
			   if(p-buf[RX2]>sizeof(buf[RX2])-4){
				   *p++='\n';
				   *p++='\r';
				   *p++='\0';
				   uartPc->printf("[RX1]%4d:%s",n2,buf[RX2]);
				   p=buf[RX2];
				   n2=0;
			   }
    	   }
    	   ptrRx2=p;
       }
       //len=uartPc->readable();
       //if(len>0)
       {
    	   char *p=ptrPc;
    	   char c;
    	   while(uartPc->getc(c)){
    		   *p=c;
			   if(*p=='\r'){
				   p++;
				   *p++='\n';
				   *p++='\0';
				   uartPc->printf("[PC]%4d:%s",p-buf[PC]-3,buf[PC]);
				   p=buf[PC];
				   if(toggleSwitchRx()){
					   uartPc->printf("uartRx1,2 enabled\n\r");
				   }else{
					   uartPc->printf("uartRx1,2 disabled\n\r");
				   }
				   break;
    		   }else
			   if(!isalnum(*p)){
				   char temp=*p;
				   //*p=(temp&0xf0)>>4+'0';
				   //*p++=(temp&0x0f)+'0';
				   *p='*';
			   }else
    		   if(*p!='\n'){
    			   p++;
			   }
			   if(p-buf[PC]>sizeof(buf[PC])-3){
				   *p++='\n';
				   *p++='\r';
				   uartPc->printf("[PC]%4d:%s",p-buf[PC],buf[PC]);
				   p=buf[PC];
			   }
    	   }
    	   ptrPc=p;
       }
   }
}
