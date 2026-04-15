// C++ code

#define PI 3.1415926535897932384626433832795
#define PAUSE 500
#define FALSE 0
#define NUM 10

  void setup()
  {
    Serial.begin(9600);
    Serial.println("");
    // int paraDEC = 78;
    //int paraBIN = 0b1001101;
    //int paraOCT = 0545701;
    //int paraHEX = 0xe45a9;
   /* int para_vec[4] = {16, 0b10001, 023, 0x1a};
    for(int i = 0 ; i < 4 ; i++)
    {
      Serial.print("Variable number ");
      Serial.print(i+1);
      Serial.print(" in binary, octal, decimal and hexadecimal forms: \n");
      Serial.println(para_vec[i], BIN);
      Serial.println(para_vec[i], OCT);
      Serial.println(para_vec[i]);
      Serial.println(para_vec[i], HEX);
    } 
    float ang_vec[5] = {0, 30, 45, 60, 90};
    float sin_val[5];
    float cos_val[5];
    int sin_val_int[5];
    int cos_val_int[5];
    for(int i = 0 ; i < 5 ; i++)
    {
      float ang_in_rad = ang_vec[i] * PI/180;
      sin_val[i] = float(sin(ang_in_rad));
      cos_val[i] = float(cos(ang_in_rad));
      sin_val_int[i] = int(sin(ang_in_rad));
      cos_val_int[i] = int(cos(ang_in_rad));
      Serial.print("The sine and cosine values of ");
      Serial.print(ang_vec[i]);
      Serial.print(" are \n");
      Serial.println(sin_val[i], 4);
      Serial.println(cos_val[i], 4);
      Serial.print("and in integers:\n");
      Serial.println(sin_val_int[i]);
      Serial.println(cos_val_int[i]);
    }
    
    byte counter = 0;
    bool LSB = FALSE;
    bool rom_bit = FALSE;
    
  	bool red_led1 = FALSE;
  	bool green_led2 = FALSE;
  	bool blue_led3 = FALSE;
    while (counter < 254)
    {
      LSB = bitRead(counter, 0);
      rom_bit = bitRead(counter, 1);
    
      red_led1 = LSB & ~rom_bit;
      green_led2 = ~LSB & rom_bit;
      blue_led3 = LSB & rom_bit;
    
      Serial.print("The red light is...");
      Serial.println(red_led1);
      Serial.print("The green light is...");
      Serial.println(green_led2);
      Serial.print("The blue light is...");
      Serial.println(blue_led3);
    
      counter = counter + 1;
      delay(PAUSE);    
    }*/
    
    int high_bound = 100;
    int low_bound = 0;
    
    long rand_num_vec[NUM];
    long sorted_num_vec[NUM + 1];
    int temp_max = low_bound;
    randomSeed(analogRead(0));
    
    for (int i = 0 ; i < NUM ; i++)
    {
      rand_num_vec[i] = random(low_bound, high_bound);
      Serial.println(rand_num_vec[i]);
      sorted_num_vec[i] = high_bound;
      //Serial.println(sorted_num_vec[i]);
    }
    /*for (int i = 1 ; i <= NUM ; i++)
    {
      for (int j = 0 ; j < NUM ; j++)
      {
        if (rand_num_vec[j] >= sorted_num_vec[i-1]) continue;
        
        if (rand_num_vec[j] > temp_max)
        {
          temp_max = rand_num_vec[j];
        }
      }
      sorted_num_vec[i] = temp_max;
      Serial.print("The sorted number ");
      Serial.print(i);
      Serial.print(" is ");
      Serial.println(sorted_num_vec[i]);
      temp_max = low_bound;
    }*/
   for (int i = 1 ; i < NUM ; i++)
    {
      x = rand_num_vec[i - 1];
      y = rand_num_vec[i];
      if (x < y)
      {
        rand_num_vec[i] = y;
        rand_num_vec[i-1] = x;
      }
      else
      {
        continue;
      }
      sorted_num_vec[i] = temp_max;
      Serial.print("The sorted number ");
      Serial.print(i+1);
      Serial.print(" is ");
      Serial.println(sorted_num_vec[i]);
      temp_max = low_bound;
    }
    
  }

void loop() {
  
  
  
}