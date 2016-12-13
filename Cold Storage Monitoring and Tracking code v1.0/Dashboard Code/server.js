var Temp=0,Hum=0,Bat=0,lat=0,lng=0;
var PubNub = require('pubnub')

pubnub = new PubNub({
        publishKey : 'pub-c-3754af37-b56d-406b-870c-424b2eba49ea',    //replace with your publish key
        subscribeKey : 'sub-c-d4cf22c8-bdf2-11e6-b490-02ee2ddab7fe'	  //replace with your subscribe Key
    })
	
function publishSampleMessage() {
        console.log("Since we're publishing on subscribe connectEvent, we're sure we'll receive the following publish.");
        var publishConfig = {
            channel : "my_channel",
            message : "Hello from PubNub Docs!"
        }
        pubnub.publish(publishConfig, function(status, response) {
            console.log(status, response);
        })
    }
       
pubnub.addListener({
    status: function(statusEvent) {
        if (statusEvent.category === "PNConnectedCategory") {
           publishSampleMessage();
        }
    },
    message: function(response) {
      console.log("New Message!!", response);
	  var json_data = JSON.stringify(response);
	  var jsondata = JSON.parse(json_data);
	  if (jsondata.message.Data=="Cold Storage Monitoring")
	  {
	  Temp=jsondata.message.Temperature;
	  Hum=jsondata.message.Humidity;
	  Bat=jsondata.message.BatteryLevel;
	  lat=jsondata.message.latlng[0];
	  lng=jsondata.message.latlng[1];
	  console.log("Data Received: ",Temp, Hum, Bat,lat,lng);
	  }
	},
    presence: function(presenceEvent) {
        // handle presence
    }
})  

console.log("Subscribing..");
pubnub.subscribe({
    channels: ['my_channel'] 
})

	
 
var express = require('express');  //web server
var app = express();

var server = require('http').createServer(app);
var io = require('socket.io').listen(server);	//web socket server

server.listen(8080); //start the webserver on port 8080
console.log('Listening on port 8080');
app.use(express.static('public')); //tell the server that ./public/ contains the static webpages

// define interactions with client
io.sockets.on('connection', function(socket){

	console.log('New Client Connected...');
	//send data to newly connected client once
	socket.emit('Temp', {'Temp': Temp});
	socket.emit('Hum', {'Hum': Hum});
	socket.emit('level', {'level': Bat});
	socket.emit('lat', {'lat': lat});
    socket.emit('lng', {'lng': lng})
   
	//send data to every connected clients after equal interval for updates
    setInterval(function(){
		io.sockets.emit('Temp', {'Temp': Temp});
		io.sockets.emit('Hum', {'Hum': Hum});
		io.sockets.emit('level', {'level': Bat});		//for battery level
		io.sockets.emit('lat', {'lat': lat});
		io.sockets.emit('lng', {'lng': lng});
		
    }, 10000);
		
});