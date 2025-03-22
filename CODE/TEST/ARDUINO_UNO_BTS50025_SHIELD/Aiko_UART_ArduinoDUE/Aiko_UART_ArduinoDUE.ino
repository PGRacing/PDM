//#include <avr/wdt.h>
#include <DueTimer.h>

void BTS3XXXEJ_init(void);
void BTF3XXXEJ_init(void);
void BTT3018EJ_init(void);
void BTT3018EJ_Callin(void);
void BTS3XXXEJ_Callin(void);
void BTF3XXXEJ_Callin(void);
int translater(char a);
void PWM_Activation(void);
void update_PWM_parameters(void);

uint8_t VIS[]="VIS", Vin[]="VN", VinDEUX[]="VI", VO[]="VO", VS[]="VS";
uint8_t rec_data, send_data[20];
int cpt_cannal=0,temp1=123, temp2=56, temp3=56, Vref1=0,Vref2=0,Vref3=0, Vref4=0, TICK=1;
uint32_t result, event_count_PWM_IC1=0, event_count_PWM_IC2=0, event_count_PWM_IC3=0, event_count_PWM_IC4=0, event_count_PWM=0, nb_pulse=0, nb_pulse_IC1=0, nb_pulse_IC2=0, nb_pulse_IC3=0, nb_pulse_IC4=0, nb_pulse_temp=0, nb_pulse_temp_IC1=0,nb_pulse_temp_IC2=0,nb_pulse_temp_IC3=0,nb_pulse_temp_IC4=0, frequency=0, dutycycle=0, ton=0, toff=0, ton_IC2=0, toff_IC2=0, ton_IC3=0, toff_IC3=0, ton_IC4=0, toff_IC4=0, ton_IC1=0, toff_IC1=0, f_IC2=0, dc_IC2=0,f_IC3=0, dc_IC3=0, f_IC4=0, dc_IC4=0, f_IC1=0, dc_IC1=0, delay_IC1=0, delay_IC2=0, delay_IC3=0, delay_IC4=0, delay_IC1_temp=0, delay_IC2_temp=0, delay_IC3_temp=0, delay_IC4_temp=0;
bool TOATE_IC1=false, TOATE_IC2=false, TOATE_IC3=false, TOATE_IC4=false,Pulse_ON=false, Pulse_IC1=false,Pulse_IC2=false, Pulse_IC3=false, PWM_IC4=false, PWM_IC1=false,PWM_IC2=false, PWM_IC3=false, Pulse_IC4=false,BTS50015_1TAD=false,BTS3011TE=false, ShieldFourDevices=false, Part_ON=false, fully_ON=false, PWM_ON=false, BTT3018EJ=false, BTS3XXXEJ=false, BTF3XXXEJ=false;
float ratio_timer = 1.578;

 
void setup() 
{
    //Timer3.attachInterrupt(PWM_Activation); Timer3.stop();
    Serial.begin(115600); Serial.setTimeout(1000);
}

void loop()
{
      // put your main code here, to run repeatedly
      char rec_data;      

      PWM_Activation();

      delayMicroseconds(1);
      if (Serial.available()>0)
      {  
          Serial.readBytes(&rec_data, 1);
          //////Data Processing
          if(ShieldFourDevices==false)
          {                         
                switch (rec_data)
                {                   
                    case 'N':
                    {              
                         if(BTT3018EJ==true || BTS3011TE==true){digitalWrite(11,1);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,1);} else if(BTS50015_1TAD==true){digitalWrite(6,1);}  
                         Part_ON=true;
                         break; 
                    }
                    case 'F':
                    {              
                         if(BTT3018EJ==true|| BTS3011TE==true){digitalWrite(11,0);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,0);} else if(BTS50015_1TAD==true){digitalWrite(6,0);}  
                         Part_ON=false;
                         break; 
                    }
                    case 'P': //Pulse ON
                    {
                        Pulse_ON=true;Part_ON=true;nb_pulse_temp=nb_pulse; update_PWM_parameters();
                        //Timer3.start(TICK);
                        break;
                    }
                    case 'Z': {fully_ON=true;break;}
                    case 'H': //Pulses number modif
                    {
                         uint8_t mil='H',cent='H', dix='H', unit='H';

                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                         nb_pulse=1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                         break;
                    }
                    case 'S': //PWM ON
                    {
                          Pulse_ON=false; Part_ON=true;PWM_ON=true; update_PWM_parameters();
                          //Timer3.start(TICK);
                          break;
                    }
                    case 'L': //PWM function OFF to write for ONE channel
                    {
                          Part_ON=false;PWM_ON=false;Pulse_ON=false; 
                          break;
                    }                 
                    case 'a':
                    {
                        uint8_t cent='a', dix='a', unit='a';
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                        dutycycle=100*translater(cent)+10*translater(dix)+translater(unit);
                        update_PWM_parameters();
                        break;
                    }
                    case 'k':
                    {
                        uint8_t dixmil='k', mil='k',cent='k', dix='k', unit='k';
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                        Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                        frequency=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                        update_PWM_parameters();
                        break;
                    }
                    ///Interface Callin
                    case '@':
                    {       
                            if(BTT3018EJ==false && BTS3XXXEJ==false && BTF3XXXEJ==false && BTS50015_1TAD==false && BTS3011TE==false){Serial.write("product");}
                            else if(BTT3018EJ==true || BTS3011TE==true){BTT3018EJ_Callin();}
                            else if(BTS3XXXEJ==true){BTS3XXXEJ_Callin();}
                            else if(BTF3XXXEJ==true) {BTF3XXXEJ_Callin();}
                            else if(BTS50015_1TAD==true){BTS50015_1TAD_Callin();}
                            break; 
                    } 
                    //case 'R':{ wdt_enable(WDTO_15MS);break; } //Reset
                    //Alternative function
                    case '<':{ pinMode(9,OUTPUT); digitalWrite(9,1);delay(500);digitalWrite(9,0); pinMode(9,INPUT);break; } //Wabco Vin and Status are disconnected
                    case '>':{ pinMode(9,OUTPUT);digitalWrite(9,1);delay(5);digitalWrite(9,0);pinMode(9,INPUT);break; } //Wabco Vin and Status are connected
                    case '/':{ pinMode(9,OUTPUT);digitalWrite(9,1);delay(10);digitalWrite(9,0);pinMode(9,INPUT);break; } //BTF3XXXEJ Reset Status
                    case '!':
                    { 
                        if(Part_ON==true){ digitalWrite(10,0);}
                        else if(PWM_ON==true){ }
                        
                        delay(5);pinMode(9,OUTPUT);digitalWrite(9,1);delay(10);digitalWrite(9,0);pinMode(9,INPUT);
                        
                        if(Part_ON==true){ digitalWrite(10,1);}
                        else if(PWM_ON==true){ } //Timer.Start
                        
                        break; 
                    }//BTF3XXXEJ Reset Trig
                    case ')':{ pinMode(11,OUTPUT);digitalWrite(11,1);break; } //BTF3XXXEJ Enable ON
                    case '%':{ pinMode(11,OUTPUT);digitalWrite(11,0);break; } //BTF3XXXEJ Enable OFF
                    ///Product selection
                    case '&':{ BTT3018EJ=true;BTT3018EJ_init();Serial.write("BTT3018EJ");break; } //Wabco Vin and Status are connected
                    case '?':{ BTS3011TE=true;BTT3018EJ_init();Serial.write("BTS3011TE");break; } //Wabco Vin and Status are connected
                    case '-':{ BTS3XXXEJ=true;BTS3XXXEJ_init();Serial.write("BTS3XXXEJ");break; } //Wabco Vin and Status are connected
                    case '_':{ BTF3XXXEJ=true;BTF3XXXEJ_init();Serial.write("BTF3XXXEJ");break; } //Wabco Vin and Status are connected
                    case '[':{ BTS50015_1TAD=true;BTS50015_1TAD_init();Serial.write("BTS50015_1TAD");break; } //Wabco Vin and Status are connected
                    case '+':{ ShieldFourDevices=true;ShieldFourDevices_init();Serial.write("Shield Four Devices");break; } //Wabco Vin and Status are connected
                }
          }
          else
          {
                switch (rec_data)
                {
                    //case '0':{wdt_enable(WDTO_15MS); break; } //Reset
                    case '@':{ShieldFourDevices_CallIN(); break;  }
                    case 'a':{digitalWrite(12,1); break; } //IC1 ON XMC_GPIO_SetOutputHigh(DIG2);
                    case 'A':{digitalWrite(11,1); break; } //IC2 ON XMC_GPIO_SetOutputHigh(DIG1);
                    case 'b':{digitalWrite(10,1); break; } //IC3 ON XMC_GPIO_SetOutputHigh(DIG4);
                    case 'B':{digitalWrite(9,1); break; } //IC4 ON XMC_GPIO_SetOutputHigh(DIG3);
                    case 'V':{delay_IC1_temp=0; event_count_PWM_IC1=0;Pulse_IC1=false; PWM_IC1=true; cpt_cannal++; break; }//IC1 PWM ON 
                    case 'R':{delay_IC2_temp=0; event_count_PWM_IC2=0;Pulse_IC2=false; PWM_IC2=true; cpt_cannal++;break; }//IC2 PWM ON 
                    case 't':{delay_IC3_temp=0; event_count_PWM_IC3=0;Pulse_IC3=false; PWM_IC3=true; cpt_cannal++; break; }//IC3 PWM ON 
                    case 'O':{delay_IC4_temp=0; event_count_PWM_IC4=0;Pulse_IC4=false; PWM_IC4=true; cpt_cannal++;break; }//IC4 PWM ON
                    case 'L':{delay_IC1_temp=0; event_count_PWM_IC1=0;Pulse_IC1=true; nb_pulse_temp_IC1=nb_pulse_IC1; cpt_cannal++;PWM_IC1=true; break; }//IC2 PWM ON
                    case 'G':{delay_IC2_temp=0; event_count_PWM_IC2=0;Pulse_IC2=true; nb_pulse_temp_IC2=nb_pulse_IC2; cpt_cannal++;PWM_IC2=true; break; }//IC2 PWM ON 
                    case 'H':{delay_IC3_temp=0; event_count_PWM_IC3=0;Pulse_IC3=true; nb_pulse_temp_IC3=nb_pulse_IC3; cpt_cannal++;PWM_IC3=true; break; }//IC3 PWM ON 
                    case 'J':{delay_IC4_temp=0; event_count_PWM_IC4=0;Pulse_IC4=true; nb_pulse_temp_IC4=nb_pulse_IC4; cpt_cannal++;PWM_IC4=true; break; }//IC4 PWM ON 
                    case 'c':{digitalWrite(12,0);Pulse_IC1=false;PWM_IC1=false; cpt_cannal--; break; } //IC1 OFF
                    case 'C':{digitalWrite(11,0);Pulse_IC2=false;PWM_IC2=false; cpt_cannal--; break; } //Turn Off IC2 PWM OFF
                    case 'e':{digitalWrite(10,0);Pulse_IC3=false;PWM_IC3=false; cpt_cannal--; break; } //Turn Off IC3 PWM OFF
                    case 'N':{digitalWrite(9,0);Pulse_IC4=false;PWM_IC4=false; cpt_cannal--; break; } //Turn Off IC4 PWM OFF
                    case ':'://Action on IC1
                    {
                          event_count_PWM_IC1=0;event_count_PWM_IC2=0;event_count_PWM_IC3=0;event_count_PWM_IC4=0;
                          uint8_t a;
                          Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                          if(a=='D')//DC change
                          {
                            uint8_t cent='D', dix='D', unit='D';
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                            
                            dc_IC1=100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='F')//F change
                          {
                            uint8_t dixmil='F', mil='F',cent='F', dix='F', unit='F';
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                            f_IC1=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='H') //Pulses number modif
                          {
                            uint8_t mil='N',cent='N', dix='N', unit='N';
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                            Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                            nb_pulse_IC1=1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }

                          break;
                    }
                    case 'T'://Action on IC2
                    {
                          uint8_t a;
                          Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                          if(a=='D')//DC change
                          {
                              uint8_t cent='D', dix='D', unit='D';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              dc_IC2=100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='F')//F change
                          {
                              uint8_t dixmil='N', mil='N',cent='N', dix='N', unit='N';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              f_IC2=2*10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit); //Factor 2 to set 
                          }
                          else if(a=='H') //Pulses number modif
                          {
                              uint8_t mil='N',cent='N', dix='N', unit='N';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              nb_pulse_IC2=1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }

                          break;
                    }
                    case '!'://Action on IC3
                    {
                          uint8_t a;
                          Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                          if(a=='D')//DC change
                          {
                              uint8_t cent='D', dix='D', unit='D';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);    
                              dc_IC3=100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='F')//F change
                          {
                              uint8_t dixmil='N', mil='N',cent='N', dix='N', unit='N';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              f_IC3=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          } 
                          else if(a=='H') //Pulses number modif
                          {
                              uint8_t mil='N',cent='N', dix='N', unit='N';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              nb_pulse_IC3=1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }

                          break;
                    }
                    case ';'://Action on IC4
                    {
                          uint8_t a;
                          Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                          if(a=='D')//DC change
                          {
                              uint8_t cent='D', dix='D', unit='D';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);          
                              dc_IC4=100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='F')//F change
                          {
                              uint8_t dixmil='F', mil='F',cent='F', dix='F', unit='F';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              f_IC4=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }
                          else if(a=='H') //Pulses number modif
                          {
                              uint8_t mil='N',cent='N', dix='N', unit='N';
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                              Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                              nb_pulse_IC4=1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                          }

                          break;
                    }                    
                    case '_'://Mode Synchro
                    {
                         char a; //Mode Synchro launcher    
                         //IC1
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);                      
                         if(a=='-'){PWM_IC1=false;} else if(a==')'){PWM_IC1=true;} //IC1 ON/OFF
                         uint8_t dixmil, mil, cent, dix, unit;
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                         delay_IC1=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='s'){nb_pulse_temp_IC1=nb_pulse_IC1; Pulse_IC1=true;}
                         else if(a=='l'){Pulse_IC1=false;} //DC PWM ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='g'){TOATE_IC1=false;} else if(a=='b'){TOATE_IC1=true;} //Turn on at the end
                         
                         //IC2
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1); //Mode Synchro launcher                         
                         if(a=='-'){PWM_IC2=false;} else if(a==')'){PWM_IC2=true;} //IC1 ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                         delay_IC2=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='s'){nb_pulse_temp_IC2=nb_pulse_IC2; Pulse_IC2=true;}
                         else if(a=='l'){Pulse_IC2=false;} //DC PWM ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='g'){TOATE_IC2=false;} else if(a=='b'){TOATE_IC2=true;} //Turn on at the end
                         
                         //IC3
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1); //Mode Synchro launcher                         
                         if(a=='-'){PWM_IC3=false;} else if(a==')'){PWM_IC3=true;} //IC1 ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                         delay_IC3=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='s'){nb_pulse_temp_IC3=nb_pulse_IC3; Pulse_IC3=true;}
                         else if(a=='l'){Pulse_IC3=false;} //DC PWM ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='g'){TOATE_IC3=false;} else if(a=='b'){TOATE_IC3=true;} //Turn on at the end
                         
                         //IC4
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1); //Mode Synchro launcher                         
                         if(a=='-'){PWM_IC4=false;} else if(a==')'){PWM_IC4=true;} //IC1 ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dixmil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&mil, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&cent, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&dix, 1);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&unit, 1);
                         delay_IC4=10000*translater(dixmil)+1000*translater(mil)+100*translater(cent)+10*translater(dix)+translater(unit);
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='s'){nb_pulse_temp_IC4=nb_pulse_IC4; Pulse_IC4=true;}
                         else if(a=='l'){Pulse_IC4=false;} //DC PWM ON/OFF
                         Serial.flush(); while(Serial.available()==0){ }  Serial.readBytes(&a, 1);
                         if(a=='g'){TOATE_IC4=false;} else if(a=='b'){TOATE_IC4=true;} //Turn on at the end
                         
                         delay_IC1_temp=delay_IC1; delay_IC2_temp=delay_IC2; delay_IC3_temp=delay_IC3; delay_IC4_temp=delay_IC4;
                         delay(10);
                         break;
                    }
                    case 'i'://Stop all the mode Synchro
                    {
                         delay_IC1_temp=0; delay_IC2_temp=0; delay_IC3_temp=0; delay_IC4_temp=0;
                         event_count_PWM_IC1=0;event_count_PWM_IC2=0;event_count_PWM_IC3=0;event_count_PWM_IC4=0;
                         digitalWrite(12,0);digitalWrite(11,0);digitalWrite(10,0);digitalWrite(9,0);
                         TOATE_IC1=false;TOATE_IC2=false;TOATE_IC3=false;TOATE_IC4=false;
                         Pulse_IC1=false;Pulse_IC2=false;Pulse_IC3=false;Pulse_IC4=false;
                         PWM_IC1=false;PWM_IC2=false;PWM_IC3=false;PWM_IC4=false; 
                         break;
                    }
               }
               if(cpt_cannal==1){ratio_timer=1.578;}
               else if(cpt_cannal==2){ratio_timer=2.023;}
               else if(cpt_cannal==3){ratio_timer=2.481;}
               else if(cpt_cannal==4){ratio_timer=2.919;}
               update_PWM_parameters();
          }              
     }
}
/////BTT3018EJ functions
void BTT3018EJ_init(void)
{
      pinMode(11, OUTPUT);//Vin
      digitalWrite(11,0); 
      pinMode(9,INPUT);//Vstatus
      frequency=1;
}

void BTT3018EJ_Callin(void)
{
      int Vstatus = 0, VI= 0;
      Vstatus = digitalRead(9);
      Serial.print("VS");
      Serial.print(Vstatus);

      if(PWM_ON==false)
      {
          VI = digitalRead(11);
          Serial.print("VI");
          Serial.print(VI);
      }
      else Serial.print("VI1");

      Vref2=4.88*analogRead(A1);
      Serial.print("VO");
      Serial.print(Vref2);
}
/////////BTS3XXXEJ
void BTS3XXXEJ_init(void)
{
      pinMode(10, OUTPUT);//Vin
      //digitalWrite(10,0); 
      pinMode(9,INPUT);//Vstatus
      frequency=1;
}

void BTS3XXXEJ_Callin(void)
{
      int Vstatus = 0, VI= 0;
      Vstatus = digitalRead(9);
      Serial.print("VS");
      Serial.print(Vstatus);

      if(PWM_ON==false)
      {
          VI = digitalRead(10);
          Serial.print("VI");
          Serial.print(VI);
      }
      else Serial.print("VI1");

      Vref2=4.88*analogRead(A1);
      Serial.print("VO");
      Serial.print(Vref2);
}
////
void BTF3XXXEJ_init(void)
{
      pinMode(10, OUTPUT);//Vin
      digitalWrite(10,0); 
      pinMode(9,INPUT);//Vstatus
      frequency=1;
}

void BTF3XXXEJ_Callin(void)
{
      int Vstatus = 0, VI= 0;
      Vstatus = digitalRead(9);
      Serial.print("VS");
      Serial.print(Vstatus);

      if(PWM_ON==false)
      {
          VI = digitalRead(10);
          Serial.print("VI");
          Serial.print(VI);
      }
      else Serial.print("VI1");

      Vref2=4.88*analogRead(A1);
      Serial.print("VO");
      Serial.print(Vref2);
}

void ShieldFourDevices_init(void)
{
      pinMode(12, OUTPUT);//Vin1
      pinMode(11, OUTPUT);//Vin2
      pinMode(10, OUTPUT);//Vin3
      pinMode(9, OUTPUT);//Vin4
      digitalWrite(12,0); 
      digitalWrite(11,0); 
      digitalWrite(10,0); 
      digitalWrite(9,0); 
}

void ShieldFourDevices_CallIN(void)
{
      Vref1=4.88*analogRead(A0);
      Vref2=4.88*analogRead(A1);
      Vref3=4.88*analogRead(A2);
      Vref4=4.88*analogRead(A3);
      Serial.print("V1");
      Serial.print(Vref1/10);
      Serial.print("V2");
      Serial.print(Vref2/10);
      Serial.print("V3");
      Serial.print(Vref3/10);
      Serial.print("V4"); 
      Serial.print(Vref4/10);                      
}

//// Shield BTS50015-1TAD
void BTS50015_1TAD_init(void){pinMode(6, OUTPUT);}

void BTS50015_1TAD_Callin(void)
{
      if(PWM_ON==false){int VI = digitalRead(6); Serial.print("VI"); Serial.print(VI);}
      else Serial.print("VI1"); 
      unsigned int Vis=4.88*analogRead(A2); Serial.print("Vis"); Serial.print(Vis);Serial.print("X");
}
/////other functions
int translater(char a)
{
      noInterrupts();interrupts(); 
      if(a=='0') return 0;
      else if(a=='1') return 1;
      else if(a=='2') return 2;
      else if(a=='3') return 3;
      else if(a=='4') return 4;
      else if(a=='5') return 5;
      else if(a=='6') return 6;
      else if(a=='7') return 7;
      else if(a=='8') return 8;
      else if(a=='9') return 9;
      else return -1;
}
//
void PWM_Activation(void) //Full stack timed function for 4 PWM action pulsed or DC
{
//  TIMER_ClearEvent(&TIMER_3);
  if(ShieldFourDevices==true) //FourShieldPWM
  {
    event_count_PWM_IC1++; event_count_PWM_IC2++; event_count_PWM_IC3++; event_count_PWM_IC4++;//Ils se désynchronise...
    if(PWM_IC1==true)
    {
      //PWM management
      if(event_count_PWM_IC1<=delay_IC1_temp){digitalWrite(12,0);}
      else if (event_count_PWM_IC1 <= (ton_IC1+delay_IC1_temp)){digitalWrite(12,1);}
      else if (event_count_PWM_IC1 <= (toff_IC1+ton_IC1+delay_IC1_temp)){digitalWrite(12,0);}
      else if(event_count_PWM_IC1 > (toff_IC1+ton_IC1)){if(nb_pulse_temp_IC1>0 && Pulse_IC1==true){nb_pulse_temp_IC1--;}digitalWrite(12,0);event_count_PWM_IC1=0;}
      //End condition
      if(PWM_IC1==false && Pulse_IC1==false){digitalWrite(12,0);event_count_PWM_IC1=0;} //End of PWM mode
      else if(nb_pulse_temp_IC1==0 && Pulse_IC1==true)
      {
        if(TOATE_IC1==true){digitalWrite(12,1);} else {TOATE_IC1=false;digitalWrite(12,0);}
        Pulse_IC1=false;PWM_IC1=false;event_count_PWM_IC1=0;
      } //End of Pulses mode
    }
    if(PWM_IC2==true)
    {
      //PWM management
      if(event_count_PWM_IC2<=delay_IC2_temp){digitalWrite(11,0);}
      else if (event_count_PWM_IC2 <= (ton_IC2+delay_IC2_temp)){digitalWrite(11,1);}
      else if (event_count_PWM_IC2 <= (toff_IC2+ton_IC2+delay_IC2_temp)){digitalWrite(11,0);}
      else if (event_count_PWM_IC2 > (toff_IC2+ton_IC2)){if(nb_pulse_temp_IC2>0 && Pulse_IC2==true){nb_pulse_temp_IC2--;} event_count_PWM_IC2=0;digitalWrite(11,0);}
      //End condition
      if(PWM_IC2==false && Pulse_IC2==false){digitalWrite(11,0);event_count_PWM_IC2=0;} //End of PWM mode
      else if(nb_pulse_temp_IC2==0 && Pulse_IC2==true)
      {
        if(TOATE_IC2==true){digitalWrite(11,1);} else {TOATE_IC2=false; digitalWrite(11,0);}
        Pulse_IC2=false;PWM_IC2=false;event_count_PWM_IC2=0;
      } //End of Pulses mode
    }
    if(PWM_IC3==true)
    {
      //PWM management
      if(event_count_PWM_IC3<=delay_IC3_temp){digitalWrite(10,0); }
      else if (event_count_PWM_IC3 <= (ton_IC3+delay_IC3_temp)){digitalWrite(10,1); }
      else if (event_count_PWM_IC3 <= (toff_IC3+ton_IC3+delay_IC3_temp)){digitalWrite(10,0); }
      else if(event_count_PWM_IC3 > (toff_IC3+ton_IC3)){if(nb_pulse_temp_IC3>0 && Pulse_IC3==true){nb_pulse_temp_IC3--;} digitalWrite(10,0); event_count_PWM_IC3=0;}
      //End condition
      if(PWM_IC3==false && Pulse_IC3==false){digitalWrite(10,0); event_count_PWM_IC3=0;} //End of PWM mode
      else if(nb_pulse_temp_IC3==0 && Pulse_IC3==true)
      {
        if(TOATE_IC3==true){digitalWrite(10,1); } else {TOATE_IC3=false; digitalWrite(10,0);}
        Pulse_IC3=false;PWM_IC3=false;event_count_PWM_IC3=0;
      } //End of Pulses mode
    }
    if(PWM_IC4==true)
    {
      //PWM management
      if(event_count_PWM_IC4<=delay_IC4_temp){ digitalWrite(9,0);}
      else if (event_count_PWM_IC4 <= (ton_IC4+delay_IC4_temp)){ digitalWrite(9,1);}
      else if (event_count_PWM_IC4 <= (toff_IC4+ton_IC4+delay_IC4_temp)){ digitalWrite(9,0);}
      else if(event_count_PWM_IC4 > (toff_IC4+ton_IC4)){if(nb_pulse_temp_IC4>0 && Pulse_IC4==true){nb_pulse_temp_IC4--;}   digitalWrite(9,0);event_count_PWM_IC4=0;}
      //End condition
      if(PWM_IC4==false && Pulse_IC4==false){event_count_PWM_IC4=0; digitalWrite(9,0);} //End of PWM mode
      else if(nb_pulse_temp_IC4==0 && Pulse_IC4==true)
      {
        if(TOATE_IC4==true){ digitalWrite(9,1);} else {TOATE_IC4=false;  digitalWrite(9,0);}
        Pulse_IC4=false;PWM_IC4=false;event_count_PWM_IC4=0;
      } //End of Pulses mode
    }

    if(PWM_IC1==false && Pulse_IC1==false && PWM_IC2==false && Pulse_IC2==false && PWM_IC3==false && Pulse_IC3==false && PWM_IC4==false && Pulse_IC4==false)
    {
        event_count_PWM_IC1=0;event_count_PWM_IC2=0;event_count_PWM_IC3=0;event_count_PWM_IC4=0; 
    }
    else {} //Timer3.start(TICK);
  }
  else //Part Standard
  {
    event_count_PWM++;
    //PWM management
    if (event_count_PWM <= (ton))
    {
        if(BTT3018EJ==true || BTS3011TE==true){digitalWrite(11,1);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,1);} else if(BTS50015_1TAD==true){digitalWrite(6,1);} 
    }
    else if (event_count_PWM <= (toff+ton))
    {
        if(BTT3018EJ==true || BTS3011TE==true){digitalWrite(11,0);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,0);} else if(BTS50015_1TAD==true){digitalWrite(6,0);} 
    }
    else if(event_count_PWM > (toff+ton)){if(PWM_ON==false){nb_pulse_temp--;} event_count_PWM=0;}
    //End condition
    if(PWM_ON==false && Pulse_ON==false)
    {
       if(BTT3018EJ==true || BTS3011TE==true){digitalWrite(11,0);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,0);} else if(BTS50015_1TAD==true){digitalWrite(6,0);} 
    } //End of PWM mode
    else if(nb_pulse_temp==0)
    {
      if(fully_ON==true)
      {
        if(BTT3018EJ==true || BTS3011TE==true){digitalWrite(11,1);} else if(BTS3XXXEJ==true || BTF3XXXEJ==true){digitalWrite(10,1);} else if(BTS50015_1TAD==true){digitalWrite(6,1);} 
        fully_ON=false;
      }
      else {Part_ON=false;}
      Pulse_ON=false;
    } //End of Pulses mode

    if(PWM_ON==false && Pulse_ON==false){event_count_PWM=0; } 
    else {} // Timer3.start(TICK);
  }
}

void update_PWM_parameters(void) //One device function
{
    float ton_base = (1650*dutycycle/frequency)/ratio_timer; float toff_base=(1650*(100-dutycycle)/frequency)/ratio_timer;
    ton=(uint32_t)ton_base; toff= (uint32_t)toff_base;
    ton_base = (1650*dc_IC1/f_IC1)/ratio_timer; toff_base=(1650*(100-dc_IC1)/f_IC1)/ratio_timer;
    ton_IC1=(uint32_t)ton_base; toff_IC1= (uint32_t)toff_base;
    ton_base = (1650*dc_IC2/f_IC2)/ratio_timer; toff_base=(1650*(100-dc_IC2)/f_IC2)/ratio_timer;
    ton_IC2=(uint32_t)ton_base; toff_IC2= (uint32_t)toff_base;
    ton_base = (1650*dc_IC3/f_IC3)/ratio_timer; toff_base=(1650*(100-dc_IC3)/f_IC3)/ratio_timer;
    ton_IC3=(uint32_t)ton_base; toff_IC3= (uint32_t)toff_base;
    ton_base = (1650*dc_IC4/f_IC4)/ratio_timer; toff_base=(1650*(100-dc_IC4)/f_IC4)/ratio_timer;
    ton_IC4=(uint32_t)ton_base; toff_IC4= (uint32_t)toff_base;
    
}
