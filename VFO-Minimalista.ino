#include <Wire.h>
#include <Adafruit_GFX.h> //Adafruit GXF Library
#include <Adafruit_SSD1306.h> // Adafruit SSD1306
#include "Rotary.h" // Biblioteca para ler o encoder rotativo
#include <Fonts/FreeMonoBold12pt7b.h> // Incluido na biblioteca GFX
#include <Fonts/FreeMono9pt7b.h> // Incluido na bibliotecaGFX
#include <si5351.h> //Etherkit Si5351

//Pino SDA do Si5351 ligado ao pino 8 do ESP32C3 Super Mini
//Pino SCL do Si5351 ligado ao pino 9 do ESP32C3 Super Mini
Si5351 si5351;

// Definir o tamanho do display OLED 
// SDA do display ligado ao pino 8 do ESP32C3 Super Mini
// SCK do display ligado ao pino 9 do ESP32C3 Super Mini

#define SCREEN_WIDTH 128  // Largura do display (em pixels)
#define SCREEN_HEIGHT 32  // Altura do display (em pixels)
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Define o pino do botao TX
#define BOTAO_TX 3

//Define o pino da saida de TX
#define SAIDA_TX 10

//Controla o estado TX/RX
bool transmitindo = false;

#define OFFSET 900000000 //Configure aqui o offset desejado

// Define o pino do botão que muda o passo da frequencia
// Pode-se usar a chave conjugada ao encoder ou um botão separado
// Ligado ao pino 2 do ESP32Super Mini
#define BOTAO_PASSO 2

// Define o tempo de pressionamento que difere um clique longo de um curto
#define HOLD_TIME 500

// Inicializa o objeto Encoder com os pinos definidos
// Pinos S1 e S2 do encoder ligados aos pinos 0 e 1 do ESP32C3 Super Mini

Rotary enc = Rotary(0, 1);


unsigned char result; //resultado da leitura do encoder
double freq_khz = 14000.00;     // Frequencia em KiloHertz
uint64_t freq_hz = 1400000000; //Frequencia em Hertz
//uint64_t offset = 0; 
int passo = 100000;  // Valor inicial do passo do encoder.
String step = "1 Khz"; //String que será impressa no display abaixo da frequncia 
int controle = 2; //Variavel de controle do passo
bool controleTX = false;

//Variáveis que controlam o pressionamento do botão de passo
unsigned long buttonPressTime = 0;
bool buttonHeld = false;

void atualizaDisplay(){

    display.clearDisplay();
    display.setFont(&FreeMonoBold12pt7b); 
    display.setCursor(0, 13);
    freq_khz = freq_hz / 100000.0;
    display.println(freq_khz, 2);
    display.setCursor(0, 31);
    display.setFont(&FreeMono9pt7b);
    display.print(step);
    display.display();  
}

void atualizaPasso(){
  if(controle == 0) { passo = 1000; step = "10 Hz"; }
  else
  if(controle == 1)  { passo = 10000; step = "100 Hz";  }
  else
  if(controle == 2) { passo = 100000; step = "1 KHz";  }
  else
  if(controle == 3)  { passo = 1000000; step = "10 KHz";  }
  else
  if(controle == 4) { passo = 10000000; step = "100 KHz"; }
  else 
  if (controle == 5) { passo = 100000000; step = "1 MHz"; }
}

void setup() {

  Serial.begin(115200);

  //Inicia o si5351 com a frequencia inicial
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(30000, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_freq(freq_hz, SI5351_CLK0);

  // Inicializa o display OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  //Inicia o botão de controle do passo
  pinMode(BOTAO_PASSO, INPUT_PULLUP);

  pinMode(BOTAO_TX, INPUT_PULLUP);
  pinMode(SAIDA_TX, OUTPUT);
  digitalWrite(SAIDA_TX, LOW);

  //Inicia o encoder
  enc.begin();

  display.clearDisplay();
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 13);
  display.println(freq_khz, 2);
  display.setCursor(0, 31);
  display.setFont(&FreeMono9pt7b);
  display.print(step);
  display.display();  
}


void loop() {

  if(digitalRead(BOTAO_TX) == LOW) transmitindo = true;
  else transmitindo = false;


  if((transmitindo == true) && (controleTX == false)){ 
      Serial.print("Transmitindo: ");
      Serial.println(transmitindo);
      Serial.print("ControleTX: ");
      Serial.println(controleTX);      

      digitalWrite(SAIDA_TX, HIGH);
      freq_hz = freq_hz + OFFSET;
      si5351.set_freq(freq_hz, SI5351_CLK0);
      controleTX = true;
  }

  if((transmitindo == false) && (controleTX == true)){
      Serial.print("Transmitindo: ");
      Serial.println(transmitindo);
      Serial.print("ControleTX: ");
      Serial.println(controleTX);  

      digitalWrite(SAIDA_TX, LOW);
      freq_hz = freq_hz - OFFSET;
      si5351.set_freq(freq_hz, SI5351_CLK0);
      controleTX = false;
  }  

  // O estado anterior do botão
  static bool lastButtonState = HIGH;  
  bool currentButtonState = digitalRead(BOTAO_PASSO);

  // Detecta o início de um pressionamento
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = millis(); // Marca o tempo em que o botão foi pressionado
    buttonHeld = false;
  }

  // Detecta o final de um pressionamento
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    unsigned long pressDuration = millis() - buttonPressTime;

    if (pressDuration < HOLD_TIME && !buttonHeld) {
      //Caso de um clique rápido      
      controle = controle - 1; 
      if(controle < 0) { controle = 0 ; }
      atualizaPasso();
      atualizaDisplay();
      buttonHeld = true;
      delay(500);
    }
  }

  // Detecta se o botão está sendo segurado
  if (currentButtonState == LOW && (millis() - buttonPressTime) >= HOLD_TIME) {
    if (!buttonHeld) { // Evita decrementos repetidos enquanto segurado
      //Caso de um clique lento
      controle = controle + 1; 
      if(controle > 5) { controle = 5 ; }
      atualizaPasso();
      atualizaDisplay();
      delay(500);
    }
  }

  lastButtonState = currentButtonState; // Atualiza o estado do botão

  // Lê a posição do encoder
  result = enc.process();
  
  // Se a posição do encoder mudou, atualizar o valor
  if (result == DIR_NONE) {

  }else if(result == DIR_CW){

    freq_hz = freq_hz + passo;
    atualizaDisplay();
    si5351.set_freq(freq_hz, SI5351_CLK0);
        
  } else if(result == DIR_CCW){
    freq_hz = freq_hz - passo;   
    atualizaDisplay();
    si5351.set_freq(freq_hz, SI5351_CLK0);    
  }

}
