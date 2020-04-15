// See https://github.com/dialogflow/dialogflow-fulfillment-nodejs
// for Dialogflow fulfillment library docs, samples, and to report issues

/*
  Software de controle (fullfilment) de uma entidade de IA de conversação e controle domótico utilizando 
    um microcontrolador ESP32.
  Autor: Jairo Ribeiro Lima - Bacharel em Ciência da Computação (UESPI)
  Fevereiro de 2020

*/

'use strict';
 
const functions = require('firebase-functions');
const {WebhookClient} = require('dialogflow-fulfillment');
const {Card, Suggestion} = require('dialogflow-fulfillment');

const admin = require('firebase-admin');
  // Your web app's Firebase configuration
  var firebaseConfig = {
    apiKey: "********************************",
    authDomain: "********************************",
    databaseURL: "********************************",
    projectId: "********************************",
    storageBucket: "********************************",
    messagingSenderId: "********************************",
    appId: "********************************"
  };
  // Initialize Firebase
  admin.initializeApp(firebaseConfig);

 
process.env.DEBUG = 'dialogflow:debug'; // enables lib debugging statements
 
exports.dialogflowFirebaseFulfillment = functions.https.onRequest((request, response) => {
  const agent = new WebhookClient({ request, response });
  console.log('Dialogflow Request headers: ' + JSON.stringify(request.headers));
  console.log('Dialogflow Request body: ' + JSON.stringify(request.body));
  
  
      function handleDevice(agent) {
      const device = agent.parameters.devices;
      const status = agent.parameters.status;
      
        if(device && status){         
         if(device == 'tudo'){
            var novoLuz,novoLed,novoVentilador, novoTomada;
            if(status =='ligar' || status =='on' || status == 'ligado' || status == 'ligada'){
                novoLuz='ligada';
                novoLed='ligado';
                novoVentilador='ligado';
                novoTomada='ligada';
            }
            else{
                novoLuz='desligada';
                novoLed='desligado';
                novoVentilador='desligado';
                novoTomada='desligada';
            }
           
            admin.database().ref('AUTOMATION/LUZ').set({
                status: novoLuz,
            });
           
            admin.database().ref('AUTOMATION/LED').set({
                status: novoLed,
            });
           
            admin.database().ref('AUTOMATION/VENTILADOR').set({
                status: novoVentilador,
            });
           
           admin.database().ref('AUTOMATION/TOMADA').set({
                status: novoTomada,
            });
           
            agent.add(`Feito!`);
            
         }
         else{
            var novo;
            if(device == 'luz'){
               if(status == 'ligar' || status == 'on' || status == 'ligada' || status == 'ligado')
                novo = "ligada";
               else
                novo = "desligada";
             }else if(device == 'tomada'){
               if(status == 'ligar' || status == 'on' || status == 'ligada' || status == 'ligado')
                novo = "ligada";
               else
                novo = "desligada";
             }else {
                if(status == 'ligar' || status == 'on' || status == 'ligado')
                    novo = 'ligado';
                else
                    novo = 'desligado';
             }

             admin.database().ref('AUTOMATION/'+device).set({
               status: novo,
              });
              agent.add(`Entendido! `+ device +' '+ novo);
         }        
        }
        else{
            agent.add('Desculpe acho que não entendi!');
           
        }   
  }
  
  function welcome(agent) {
    agent.add(`Bem vindo!`);
    agent.add(`Olá!`);
    agent.add(`Como posso ajudar?`);
  }
 
  function fallback(agent) {
    agent.add(`I didn't understand`);
    agent.add(`I'm sorry, can you try again?`);
  }

  function status(agent){
    const device = agent.parameters.devices;
    const status = agent.parameters.status;
    if(device){
                return admin.database().ref('/AUTOMATION/'+device+'/status').once('value').then(function(snapshot) {
                var valor = snapshot.val();
                console.log('valor ='+valor);
                console.log('device = '+device);
                if(device == 'temperatura'){
                  console.log('entrou');
                  agent.add('O valor da temperatura no momento é de '+valor+' graus Celsius');
                }else if(device == 'umidade')
                      agent.add(`O valor da umidade no momento é de `+valor+` porcento`);
                    else if(device == 'luz')
                        agent.add(`A luz está `+valor);
                      else if(device == 'led')
                          agent.add(`O led está `+valor);
                        else if(device == 'ventilador')
                                agent.add(`O ventilador está `+valor);
                            else if(device == 'tomada')
                                agent.add(`A tomada está `+valor);
              });
            }
            else{
                agent.add(`Desculpe, acho não entendi, você poderia repetir?`);
            }
  }

  function saveName(agent) {
    var nameParam = agent.parameters.nome;
    //const context = agent.getContext('awaiting_name_confirm');
    var nm = nameParam;
    
    agent.add(`Obrigado, ` + nm);
    //sendSlackMessage(name);
    
    return admin.database().ref('/nomes').push({nome: nm}).then((snapshot) => {
    // Redirect with 303 SEE OTHER to the URL of the pushed object in the Firebase console.
    console.log('database write sucessful: ' + snapshot.ref.toString());
  });
  }
  /*
  function hora(agent){
    var dt = agent.parameters.horas;
    var hr = new Date();
    //const timeZone = 'America/Buenos_Aires'; 
  	//const timeZoneOffset = '-3:00'; 
  	//const dateTimeStart  = new Date(Date.parse('T' + agent.parameters.horas.split('T')[1].split('-')[0] + timeZoneOffset));
    var h = hr.getHours();
    agent.add('A hora no momento é :' + dt);
  }
 */
  function cad(agent){
    var newKey = admin.database().ref().child('CADASTRO').push().key;
    admin.database().ref('CADASTRO/'+newKey).set({
      Nome: agent.parameters['nome'],
      Telefone: agent.parameters['telefone'],
    });
    agent.add('Obrigado ' + agent.parameters.nome+ ', cadastro realizado com sucesso!');
  }
  
  function raiz(agent){
    var x = agent.parameters.number;
    var total;
    if(x>=0){
      total = Math.sqrt(x);
        agent.add('Segundo os meus cálculos, a raíz quadrada de '+ x + ' é '+ total);
        return admin.database().ref('/RAÍZES').push({raíz: total}).then((snapshot) => {
    // Redirect with 303 SEE OTHER to the URL of the pushed object in the Firebase console.
    console.log('database write sucessful: ' + snapshot.ref.toString());
  });
    }else{
      agent.add('Não existe raíz de números negativos');
    }
  }
  
  function soma(agent){
    var adicao = agent.parameters.number1 + agent.parameters.number2;
    agent.add('Segundo os meus cálculos, a resposta é: ' + adicao);
    
    return admin.database().ref('/somas').push({soma: adicao}).then((snapshot) => {
    // Redirect with 303 SEE OTHER to the URL of the pushed object in the Firebase console.
    console.log('database write sucessful: ' + snapshot.ref.toString());
  });
  }
  
  function sub(agent){
    var dif = agent.parameters.number1 - agent.parameters.number2;
    agent.add('Segundo os meus cálculos, a resposta é: ' + dif);
    
    return admin.database().ref('/subtracoes').push({subtracao: dif}).then((snapshot) => {
    // Redirect with 303 SEE OTHER to the URL of the pushed object in the Firebase console.
    console.log('database write sucessful: ' + snapshot.ref.toString());
  });
  }

  // // Uncomment and edit to make your own intent handler
  // // uncomment `intentMap.set('your intent name here', yourFunctionHandler);`
  // // below to get this function to be run when a Dialogflow intent is matched
   function GName(agent) {
     agent.add(`This message is from Dialogflow's Cloud Functions for Firebase editor!`);
     agent.add(new Card({
         title: `Title: this is a card title`,
         imageUrl: 'https://developers.google.com/actions/images/badges/XPM_BADGING_GoogleAssistant_VER.png',
         text: `This is the body text of a card.  You can even use line\n  breaks and emoji! 💁`,
         buttonText: 'This is a button',
         buttonUrl: 'https://assistant.google.com/'
       })
    );
     agent.add(new Suggestion(`Quick Reply`));
     agent.add(new Suggestion(`Suggestion`));
     agent.setContext({ name: 'weather', lifespan: 2, parameters: { city: 'Rome' }});
   }

  // // Uncomment and edit to make your own Google Assistant intent handler
  // // uncomment `intentMap.set('your intent name here', googleAssistantHandler);`
  // // below to get this function to be run when a Dialogflow intent is matched
   function googleAssistantHandler(agent) {
     let conv = agent.conv(); // Get Actions on Google library conv instance
     conv.ask('Hello from the Actions on Google client library!'); //Use Actions on Google library
     agent.add(conv); // Add Actions on Google library responses to your agent's response
   }
  // // See https://github.com/dialogflow/fulfillment-actions-library-nodejs
  // // for a complete Dialogflow fulfillment library Actions on Google client library v2 integration sample

  // Run the proper function handler based on the matched Dialogflow intent name
  let intentMap = new Map();
  intentMap.set('firebaseControl', handleDevice);
  intentMap.set('Default Welcome Intent', welcome);
  intentMap.set('Default Fallback Intent', fallback);
  intentMap.set('GetName', saveName);
  intentMap.set('adicao', soma);
  intentMap.set('subtracao', sub);
  //intentMap.set('hora', hora);
  intentMap.set('cadastro', cad);
  intentMap.set('raizes', raiz);
  intentMap.set('status', status);
  intentMap.set('your intent name here', googleAssistantHandler);
  agent.handleRequest(intentMap);
});