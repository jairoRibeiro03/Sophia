/*
    Software de controle dispositivos (relés e sensores) por uma entidade de IA(DialogFlow) de conversação e controle domótico utilizando
    um microcontrolador ESP32. O código já se encontra adaptado para upload utilizando o protocolo de envio via WiFi (OTA).
  Autor: Jairo Ribeiro Lima - Bacharel em Ciência da Computação (UESPI)
  Período: Fevereiro de 2020 - ***

*/

#include <Arduino.h> // Lib padrão
#include "esp_task_wdt.h" // Lib do watchdog
#include <IOXhop_FirebaseESP32.h> // Lib do Firebase
#include "DHT.h" // Lib do sensor de temperatura e umidade DHTxx

// Libs OTA
#include <WiFi.h> //lib para configuração do Wifi
#include <ArduinoOTA.h> //lib do ArduinoOTA 
#include <ESPmDNS.h> //lib necessária para comunicação network
#include <WiFiUdp.h> //lib necessária para comunicação network

#define DHTTYPE DHT11   // Define o tipo de sendor DHT, no caso o 11
#define DHTPIN 3        // Pino do sensor DHT11

// Pinos dos relés
#define luzLed     2
#define luz        18
#define ventilador 5
#define tomada     17

DHT dht(DHTPIN, DHTTYPE); // Objeto para controle do sendor DHT11

#define SEND_DATA_INTERVAL 5000 // Intervalo de envios de temperatura e umidade (5 segundos)

// Url/Key do Firebase
#define FIREBASE_HOST "https://*****.firebaseio.com/"
#define FIREBASE_AUTH "**********"

// Nome da rede WiFi e senha

const char *ssid     = "your ssid";
const char *password = "your password";

//const char *ssid     = "*******";
//const char *password = "********";

// Valores de temperatura e umidade que serão enviados para o Firebase
String temp = "", humd = "";


/* 		########## Protótipo das funções auxiliares ####################*/
void wifiBegin(); // Função que inicia o WiFi
void OTA_init(); // Função que inicialia as funções padrão para a transmissão de dados pelo ar (OTA).
void readClimate(); // Função que lê os valores do sensor de temperatura e umidade.
String executeCommandFromFirebase(String cmd, String value);// Função que executa um comando ativando e desativando os relés
void syncFirebase(String value); // Função que ativa/desativa todos os relés de acordo com a mensagem recebida ao iniciar o Firebase (Sincronizando as saídas do ESP com os valores atuais do Firebase)
void firebaseBegin(); // Função que inicia a conexão com o Firebase
void sendSensorData(void *p); // Task que envia os dados de temperatura e umidadade para o Firebase de tempo em tempo



void setup()
{
  // Iniciamos a serial
  Serial.begin(115200);

  // Inicializamos o watchdog com 15 segundos de timeout
  esp_task_wdt_init(15, true);

  // Setamos os pinos dos relés como saída
  pinMode(luz, OUTPUT);
  pinMode(luzLed, OUTPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(tomada, OUTPUT);
  pinMode(DHTPIN, INPUT);

  // Iniciamos o WiFi
  wifiBegin();

  //define wifi como station (estação)
  WiFi.mode(WIFI_STA);

  // Iniciamos o sensor
  dht.begin();

  OTA_init(); // Funções OTA

  // Iniciamos a conexão com o Firebase
  firebaseBegin();
  // Iniciamos a task de envio de temperatura e umidade
  xTaskCreatePinnedToCore(sendSensorData, "sendSensorData", 10000, NULL, 1, NULL, 0);
}

void loop()
{
  ArduinoOTA.handle();// Função que verifica se existe dados para upload
}

// Função que inicia o WiFi
void wifiBegin()
{
  // Iniciamos o WiFi
  WiFi.begin(ssid, password);

  Serial.println("Wifi Connecting");

  // Enquanto não estiver conectado exibimos um ponto
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    esp_task_wdt_reset();
    delay(1000);
  }
  Serial.print(" CONEXÃO WIFI OK (");
  Serial.print(WiFi.localIP());
  Serial.println(")");
}

void OTA_init() 
{
  // Iniciamos o WiFi
  //WiFi.begin(ssid, password);
  //enquanto o wifi não for conectado aguarda
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    //caso falha da conexão, reinicia wifi
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // A porta fica default como 3232
  // ArduinoOTA.setPort(3232);

  // Define o hostname (opcional)
  ArduinoOTA.setHostname("ESP32");

  // Define a senha (opcional)
  ArduinoOTA.setPassword("senha123");

  // É possível definir uma criptografia hash md5 para a senha usando a função "setPasswordHash"
  // Exemplo de MD5 para senha "admin" = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  //define o que será executado quando o ArduinoOTA iniciar
  ArduinoOTA.onStart( startOTA ); //startOTA é uma função criada para simplificar o código

  //define o que será executado quando o ArduinoOTA terminar
  ArduinoOTA.onEnd( endOTA ); 

  //define o que será executado quando o ArduinoOTA estiver gravando
  ArduinoOTA.onProgress( progressOTA ); 

  //define o que será executado quando o ArduinoOTA encontrar um erro
  ArduinoOTA.onError( errorOTA );

  //inicializa ArduinoOTA
  ArduinoOTA.begin();

  
  Serial.println("Ready");
  Serial.print("IP address: ");//exibe pronto e o ip utilizado pelo ESP
  Serial.println(WiFi.localIP());
}
//funções de exibição dos estágios de upload (start, progress, end e error) do ArduinoOTA
void startOTA()
{
  String type;

  //caso a atualização esteja sendo gravada na memória flash externa, então informa "flash"
  if (ArduinoOTA.getCommand() == U_FLASH)
    type = "flash";
  else  //caso a atualização seja feita pela memória interna (file system), então informa "filesystem"
    type = "filesystem"; // U_SPIFFS

  //exibe mensagem junto ao tipo de gravação
  Serial.println("Start updating " + type);
  /*
    digitalWrite(ledVerde,HIGH);
    delay(300);
    digitalWrite(ledVerde,LOW);
    delay(300);
    digitalWrite(ledVerde,HIGH);
    delay(300);
    digitalWrite(ledVerde,LOW);
    delay(300);
  */
}

//exibe mensagem
void endOTA()
{
  Serial.println("\nEnd");
}

//exibe progresso em porcentagem
void progressOTA(unsigned int progress, unsigned int total)
{
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

//caso aconteça algum erro, exibe especificamente o tipo do erro
void errorOTA(ota_error_t error)
{
  Serial.printf("Error[%u]: ", error);

  if (error == OTA_AUTH_ERROR)
    Serial.println("Auth Failed");
  else if (error == OTA_BEGIN_ERROR)
    Serial.println("Begin Failed");
  else if (error == OTA_CONNECT_ERROR)
    Serial.println("Connect Failed");
  else if (error == OTA_RECEIVE_ERROR)
    Serial.println("Receive Failed");
  else if (error == OTA_END_ERROR)
    Serial.println("End Failed");
}


// Função que lê os valores do sensor
void readClimate()
{
  int h = dht.readHumidity();
  int t = dht.readTemperature();

  if (t < 900)
    temp = String(t);
  else
    temp = "";

  if (h < 900)
    humd = String(h);
  else
    humd = "";
}

// Função que executa um comando ativando e desativando os pinos
String executeCommandFromFirebase(String cmd, String value)
{
  // Retira a primeira barra
  if (cmd.charAt(0) == '/')
    cmd = cmd.substring(1);

  // Exibe na serial o comando e o valor
  Serial.println("comando: " + cmd + " <");
  Serial.println("valor: " + value + " >");

  // Caso for um comando referente a lampada incandescente
  if (cmd.equals("AUTOMATION/LUZ"))
  {
    // Se o valor for "LIGADA"
    if (value.equals("{\"STATUS\":\"LIGADA\"}"))
      digitalWrite(luz, HIGH);
    else // Caso contrário desativamos o relé
      digitalWrite(luz, LOW);
  }
  else
    // Caso for um comando referente a lampada led
    if (cmd.equals("AUTOMATION/LED"))
    {
      if (value.equals("{\"STATUS\":\"LIGADO\"}")) {
        Serial.println("ON");
        digitalWrite(luzLed, HIGH);
      }
      else { // Caso contrário desativamos o pino
        Serial.println("OFF");
        digitalWrite(luzLed, LOW);
      }
    }
    else if (cmd.equals("AUTOMATION/VENTILADOR"))
    {
      if (value.equals("{\"STATUS\":\"LIGADO\"}"))
        digitalWrite(ventilador, HIGH);
      else // Caso contrário desativamos o pino
        digitalWrite(ventilador, LOW);
    } else if (cmd.equals("AUTOMATION/TOMADA"))
    {
      if (value.equals("{\"STATUS\":\"LIGADA\"}"))
        digitalWrite(tomada, HIGH);
      else // Caso contrário desativamos o pino
        digitalWrite(tomada, LOW);
    }
    else // Se não entrou em nenhum "if" acima, retornamos uma mensagem de comando inválido
      return "Invalid command";

  // Se for um comando válido retornamos a mensagem "OK"
  return  "OK";
}

// Função que ativa/desativa todos os relés de acordo com a mensagem recebida ao iniciar o Firebase (Sincronizando as saídas do ESP com os valores atuais do Firebase)
void syncFirebase(String value) 
{
  /*
    EXEMPLO DE MENSAGEM RECEBIDA:
    "LED":{"STATUS":"LIGADO"},"LUZ":{"STATUS":"LIGADA"},"TEMPERATURA":{"STATUS":"26.27"},"UMIDADE":{"STATUS":"36.76"},"VENTILADOR":{"STATUS":"LIGADO"}}}
  */

  // Obtendo a substring referente ao led
  String aux = value.substring(value.indexOf("LED"));

  int posVar = aux.indexOf("STATUS\":\"") + 9;
  String status = "";

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status LED:" + status);

  // Sincronizando saída referente a lampada led
  if (status.equals("LIGADO")) {
    Serial.println("status sincronizado(led on)");
    digitalWrite(luzLed, HIGH);
  }
  else if (status.equals("DESLIGADO")) {
    Serial.println("status sincronizado(led off)");
    digitalWrite(luzLed, LOW);
  }
  status = "";

  // Obtendo a substring referente a lampada incandescente
  aux = value.substring(value.indexOf("LUZ"));
  posVar = aux.indexOf("STATUS\":\"") + 9;

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status LUZ:" + status);
  // Sincronizando saída referente a lampada incandescente
  if (status.equals("LIGADA")) {
    Serial.println("status sincronizado(lamp on)");
    digitalWrite(luz, HIGH);
  }
  else if (status.equals("DESLIGADA")) {
    Serial.println("status sincronizado(lamp off)");
    digitalWrite(luz, LOW);
  }
  status = "";

  // Obtendo a substring referente ao ventilador
  aux = value.substring(value.indexOf("VENTILADOR"));
  posVar = aux.indexOf("STATUS\":\"") + 9;

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status VENTILADOR:" + status);
  // Sincronizando saída referente ao ventilador
  if (status.equals("LIGADO")) {
    Serial.println("status sincronizado(vent on)");
    digitalWrite(ventilador, HIGH);
  }
  else if (status.equals("DESLIGADO")) {
    Serial.println("status sincronizado(vent off)");
    digitalWrite(ventilador, LOW);
  }
  // Obtendo a substring referente a tomada
  aux = value.substring(value.indexOf("TOMADA"));

  posVar = aux.indexOf("STATUS\":\"") + 9;
  status = "";

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status TOMADA:" + status);

  // Sincronizando saída referente a lampada led
  if (status.equals("LIGADA")) {
    Serial.println("status sincronizado(tomada on)");
    digitalWrite(tomada, HIGH);
  }
  else if (status.equals("DESLIGADA")) {
    Serial.println("status sincronizado(tomada off)");
    digitalWrite(tomada, LOW);
  }
  status = "";

  // Obtendo a substring referente a temperatura
  aux = value.substring(value.indexOf("TEMPERATURA"));

  posVar = aux.indexOf("STATUS\":\"") + 9;
  status = "";

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status TEMPERATURA:" + status);
  Serial.println("status sincronizado(temperatura ok)");
  status = "";

  // Obtendo a substring referente a umidade
  aux = value.substring(value.indexOf("UMIDADE"));

  posVar = aux.indexOf("STATUS\":\"") + 9;
  status = "";

  for (int i = posVar; aux.charAt(i) != '\"'; i++)
    status += aux.charAt(i);

  Serial.println("Status UMIDADE:" + status);
  Serial.println("status sincronizado(umidade  ok)");
  status = "";
}

// Função que inicia a conexão com o Firebase
void firebaseBegin()
{
  // Iniciamos a comunicação Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // Setamos o callback (Executado a cada alteração do firebase)
  Firebase.stream("/", [](FirebaseStream stream)
  {
    String path, value;
    Serial.println("<<<<<*");
    // Se o evento que vem do callback é de alteração "put"
    if (stream.getEvent() == "put")
    {
      // Obtemos os valores de path e value
      path = stream.getPath();
      value = stream.getDataString();
      Serial.println(">>>>");
      // Deixamos em maiúsculo
      path.toUpperCase();
      value.toUpperCase();
      //Serial.print(value);
      // Se for a mensagem inicial enviada pelo firebase assim que o ESP inicia sua comunicação
      if (value.startsWith("{\"AUTOMATION\""))
      {
        // Sincronizamos as saídas para os relés de acordo com a mensagem recebida
        syncFirebase(value);
        //Serial.println(value);
        Serial.print("Firebase sincronizado");
        Serial.println(" \o/");
      }
      else
        // Se não for uma mensagem referente a temperatura e umidade, executamos um comando e exibimos na serial
        if (!path.equals("/AUTOMATION/UMIDADE/STATUS") && !path.equals("/AUTOMATION/TEMPERATURA/STATUS"))
          executeCommandFromFirebase(path, value);
    }
  });
}

// Task que envia os dados de temperatura e umidadade para o Firebase de tempo em tempo
void sendSensorData(void *p)
{
  // Adicionamos a tarefa na lista de monitoramento do watchdog
  esp_task_wdt_add(NULL);

  while (true)
  {
    // Resetamos o watchdog
    esp_task_wdt_reset();
    // Lê os valores do sensor
    readClimate();

    // Se foi possível ler o sensor
    if (temp != "" && humd != "")
    {
      // Enviamos para o Firebase
      Firebase.setString("/AUTOMATION/temperatura/status", temp);
      Firebase.setString("/AUTOMATION/umidade/status", humd);
      Serial.println("sent");
    }
    // Aguardamos um tempo de 5 segundos
    delay(SEND_DATA_INTERVAL);
  }
}
