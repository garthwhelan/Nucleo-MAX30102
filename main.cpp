#include "mbed.h"
#include <stdio.h>
#include <math.h>

// Serial connection
RawSerial pc(USBTX, USBRX);

// LED to display status
DigitalOut led1(LED1);

I2C i2c(PB_9,PB_8);
//InterruptIn a(PA_9);
DigitalIn din(PA_9);
struct pair {
    uint32_t A;
    uint32_t B;    
};

class max30102 {
    I2C *i2c;
    //InterruptIn *a;
    int max30102_addr;
    public:
        uint32_t data_IR[16];
        uint32_t data_red[16];
                
        max30102 (I2C * i2ca) {
            i2c=i2ca;
            i2c->frequency(400000);   
            max30102_addr = 0xAE;
            //a=b;
        }
        
        int write(uint8_t reg, uint8_t* buff, int len) {
            int res;
            char tmp[len+1];
            tmp[0]=(char)reg;
            memcpy(tmp+1,buff,len);
            i2c->start();
            res=i2c->write(max30102_addr,tmp,len+1);
            i2c->stop();
            return res;
        }
        
        int read_reg(uint8_t reg, uint8_t* buff, int len) {
            int res;
            i2c->start();
            res=i2c->write(max30102_addr,(char*)&reg,1);
            i2c->start();
            res+=(i2c->read(max30102_addr,(char*)buff,len)<<1);
            i2c->stop();
            return res;            
        }
        
        int write_check(uint8_t reg, uint8_t* buff, int len) {
            uint8_t buff2[len];
            return (write(reg,buff,len)||read_reg(reg,buff2,len)||memcmp(buff,buff2,len));
        }
        
        void init() {
    
            uint8_t tmp_buff[3];
label:      
            wait_ms(50);
            tmp_buff[0] = 0x40;
            if(write(0x09,tmp_buff,1)) {
                pc.printf("A");
                goto label;    
            }
            wait_ms(50);
      
            tmp_buff[0] = 0x40;
            if(write_check(0x02,tmp_buff,1)) {
                pc.printf("B");
                goto label;}//set new data interrupt
                          
            tmp_buff[0] = 0x00;             
            if(write_check(0x08,tmp_buff,1)) {
                pc.printf("C");
                goto label;}//average 8, no rollover, 'almost full' at 32
            
            tmp_buff[0] = 0x03;//SpO2 mode
            tmp_buff[1] = 0x43;//ACD range 2000nA,400SPS, 400uS pulse width
            if(write_check(0x09,tmp_buff,2)) {
                pc.printf("D");
                goto label;}
                
            tmp_buff[0] = 0x5F;//12mA
            tmp_buff[1] = 0x5F;
            if(write_check(0x0C,tmp_buff,2)) {
                pc.printf("E");
                goto label;}//LED current
    
            tmp_buff[0] = 0x01;
            tmp_buff[1] = 0x00;
            tmp_buff[2] = 0x00;
            write(0x04,tmp_buff,3);//clear FIFO
        }
    /*
        float read_temp() {
            return 0.0;
        }
        */
        
        int read_ADC(int n) {
            uint8_t loc_buff[n*6];
            i2c->start();
            char tmp[1] = {0x07};
            i2c->write(max30102_addr,tmp,1);
            i2c->start();
            i2c->read(max30102_addr,(char*)loc_buff,n*6);
            i2c->stop();
            /*for(int i=0; i < n; i++) {
                if(loc_buff[2+i*6]>3) {
                    pc.printf("error");
                }
                if(loc_buff[5+i*6]>3) {
                    pc.printf("error");
                }    
            }*/
            //tmp_buff[0] = 0x01;
            //tmp_buff[1] = 0x00;
            //tmp_buff[2] = 0x00;
            //write(0x04,tmp_buff,3);//clear FIFO
            for(int i=0; i < n; i++) {
                loc_buff[i*6]=loc_buff[i*6]&3;
                loc_buff[3+i*6]=loc_buff[3+i*6]&3;
                data_IR[i]=(((uint32_t)loc_buff[i*6])<<16)|(((uint32_t)loc_buff[1+i*6])<<8)|((uint32_t)loc_buff[2+i*6]);//not sure if in right order
                data_red[i]=(((uint32_t)loc_buff[3+i*6])<<16)|(((uint32_t)loc_buff[4+i*6])<<8)|((uint32_t)loc_buff[5+i*6]);
            }
            return 0;    
        }
        
    
};

event_callback_t fp;

void handler(int a) {
    return; //not sure if I need a fake function or not    
}

int main()
{
    led1=1;
    max30102 max(&i2c);   
    fp.attach(handler);
    pc.baud(38400);
    max.init();
    //pc.printf("start\n\r");
    uint8_t tmp_buff[3];
    while(1) {  
    //for(int i = 0; i < 100; i++) {
        //ax.read_reg(0x04,tmp_buff,3);
        //pc.printf("%i %x %x %x\n\r",i,tmp_buff[0],tmp_buff[1],tmp_buff[2]);
        max.read_reg(0x00,tmp_buff,1);
        if(tmp_buff[0]&0x40) {
        max.read_ADC(1);
        //max.read_reg(0x00,tmp_buff,0);
        //pc.printf("%i\n\r",tmp_buff[0]);
        pc.printf("%i\n\r",max.data_IR[0]);
        pc.printf("%i\n\r",max.data_red[0]);
        }
        wait_ms(1);        
    }
    //while(1) {
        /*if(!din.read()) {
            led1=!led1;
            max.read_ADC(16);// const below?
            pc.write((uint8_t*)max.data_IR,64,fp);
            pc.write((uint8_t*)max.data_red,64,fp);    
        }*/   
    }
