
//---------------------------------------------------------------------------
// display/hide tab
function openTab(evt, tabName) {
  // Declare all variables
  var i, tabcontent, tablinks;

  // Get all elements with class="tabcontent" and hide them
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tablinks" and remove the class "active"
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab, and add an "active" class to the button that opened the tab
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}

//---------------------------------------------------------------------------
// create form

function updateSensorCount(inc)
{
  var counter = document.getElementById('nsensors');
  var n = parseInt(counter.value); //innerHTML
  counter.value = n + inc;
}

document.getElementById('add_sensor').onclick = function(){ addSensor(); }

// add a sensor
function addSensor(sensor)
{
  var counter = document.getElementById('nsensors');
  var n = parseInt(counter.value); //innerHTML
  
  // snesor name
  var sensorId = "sensor[" + n + "]";
  
  var div = document.createElement("div");
  div.id = sensorId;
  div.className = 'sensor'
  
  // add '-' button
  var btn = document.createElement("input");
  btn.type = "button";
  btn.value = "-";
  btn.onclick = function()
  {
    document.getElementById("sensors").removeChild(document.getElementById(sensorId));
    updateSensorCount(-1);
  }
  div.appendChild(btn);
  
  //var nameName = sensorId+"[name]";
  var lblName = document.createElement("label");
  //lblName.htmlFor = nameName;
  lblName.innerHTML = "Name:";
  var inName = document.createElement("input");
  inName.name = "name";
  inName.type = "text";
  inName.size = "5";
	inName.required = true;
	if(sensor) inName.value = sensor.name;
  
  // sensor type
  var typeName = sensorId + "[type]";
  var lblType = document.createElement("label");
  //lblType.htmlFor = typeName;
  lblType.innerHTML = "Type:"
  var selType = document.createElement("select");
  selType.name = "type"; //typeName;
  
  // sensor type definition
  const sensor_types = new Map([
    ['', ['']]
    , ['HC-SR04', ['HC-SR04', ['pinTrig', 'pinEcho']]]
    , ['DHT11',   ['DHT11, temp/humi', ['pinData']]]
    , ['VL53L0X', ['VL53L0X, laser']]
  ]);
  
  // PINS definition
	// https://esp8266-shop.com/esp8266-guide/esp8266-nodemcu-pinout/
  const pin_values = new Map([
    ['', 255]
		// , ['A0', ]
    , ['D0', 16]  // LED_BUILTIN
		, ['D1', 5]
    , ['D2', 4]
    , ['D3', 0]
    , ['D4', 2]
    , ['D5', 14]
    , ['D6', 12]
    , ['D7', 13]
    , ['D8', 15]
    , ['RX', 3]
    , ['TX', 1]
  ]);

	// populate the sensor type options
  for (let [key, value] of sensor_types)
  {
    var option = document.createElement("option");
    option.value = key;
    option.text = value[0]; //val.charAt(0).toUpperCase() + val.slice(1);
    selType.appendChild(option);
  }

  // group together
  div.appendChild(lblName);
  div.appendChild(inName);
  div.appendChild(lblType);
  div.appendChild(selType);
  
  // add pins div
  var pins = "pins[" + n + "]";
  var div_pins = document.createElement("div");
  div_pins.id = pins;
	div_pins.name = "pins";

  // sensor type change event, create corresponding pins
  selType.onchange = function(){
    //alert(selType.value);
    
    // clear pins div
    div_pins.innerHTML = '';
  
    // reset pins div per sensor type
    var vsensor = sensor_types.get(selType.value);
    if(vsensor.length > 1)
    {
			// having pins definition
      for (const pinName of vsensor[1])
      {
        // create pin label
        //var pinName = sensorId + "[pin][" + i + "]";
        var lblPin = document.createElement("label");
        lblPin.htmlFor = pinName;
        lblPin.innerHTML = pinName + ':';
        
        // create pin select options
        var selPin = document.createElement("select");
        selPin.name = pinName;
        for (let [key, value] of pin_values)
        {
          var option = document.createElement("option");
          option.text = key;
          option.value = value; //val.charAt(0).toUpperCase() + val.slice(1);
          selPin.appendChild(option);
        }
				if(sensor) selPin.value = sensor.pins[pinName];
        
        // add pins to div_pins
        div_pins.appendChild(lblPin);
        div_pins.appendChild(selPin);
      }
    }
  };

	// update dom
	div.appendChild(div_pins);
  document.getElementById("sensors").appendChild(div);
  updateSensorCount(1);
	
	if(sensor)
	{
		selType.value = sensor.type;
		selType.onchange();
	}

/*  
  for (let i = 0; i < 3; i++)
  {
    var pinName = sensor + "[pin][" + i + "]";
    var lblPin = document.createElement("label");
    lblPin.htmlFor = pinName;
    lblPin.innerHTML = "Pin:"
    var selPin = document.createElement("select");
    selPin.name = "pin"; //pinName;
    for (let [key, value] of pin_values)
    {
      var option = document.createElement("option");
      option.text = key;
      option.value = value; //val.charAt(0).toUpperCase() + val.slice(1);
      selPin.appendChild(option);
    }
    div.appendChild(lblPin);
    div.appendChild(selPin);
  }
*/
}

// get json data from web config
function getConfigJson()
{
	const data = {};
	data.module = document.getElementById('module').value;
	
	data.wifi={'ssid':document.getElementById('ssid').value};
	data.wifi.pass = document.getElementById('pass').value;
	
	data.ip = document.getElementById('ip').value;
	data.gateway = document.getElementById('gateway').value;
	data.appass = document.getElementById('apPass').value;
	data.otapass = document.getElementById('otaPass').value;
	
	data.mqtt = {'server': document.getElementById('mqttServer').value};
	data.mqtt.port = Number(document.getElementById('mqttPort').value);
	data.mqtt.user = document.getElementById('mqttUser').value;
	data.mqtt.pass = document.getElementById('mqttPass').value;
	
	data.sensors = [];
	
  var sensors = document.getElementById("sensors");
  for(sitm of sensors.childNodes)
	{
		var sensor = {};
		for(itm of sitm.childNodes)
		{
      if (itm.name == 'name') sensor.name = itm.value;
      else if (itm.name == 'type') sensor.type = itm.value;
      else if (itm.name == 'pins')
      {
				var pins = {};
				var hasPin = false;
				for(pin of itm.childNodes)
				{
					if(pin.tagName === 'SELECT') // make sure it is select, not labels
					{
						pins[pin.name] = Number(pin.value);
						hasPin = true;
					}
				}
				
				if(hasPin) sensor.pins = pins;
      }
		}
		data.sensors.push(sensor);
	}
	
	return data;
}

// set json data from web config
function setConfigJson(js)
{
	document.getElementById('module').value = js.module;
	
	document.getElementById('ssid').value 		= js.wifi.ssid;
	document.getElementById('pass').value 		= js.wifi.pass;
	document.getElementById('ip').value 		 	= js.ip;
	document.getElementById('gateway').value 	= js.gateway;
	document.getElementById('apPass').value 	= js.appass;
	document.getElementById('otaPass').value 	= js.otapass;
	
	document.getElementById('mqttServer').value = js.mqtt.server;
	document.getElementById('mqttPort').value 	= js.mqtt.port;
	document.getElementById('mqttUser').value 	= js.mqtt.user;
	document.getElementById('mqttPass').value 	= js.mqtt.pass;
	
	// reset sensors
  var sensors = document.getElementById("sensors");
	sensors.innerHTML = '';
	
	// reset counter
	var counter = document.getElementById('nsensors');
  counter.value = 0;
	
	for(const sensor of js.sensors)
	{
		addSensor(sensor);
	}
}


//---------------------------------------------------------------------------
// AJAX requests

var get_config = document.getElementById('get_config')
get_config.onclick = function(event){ getConfig(); }

async function getConfig() {
	try {
		const response = await fetch('/api/config/get');
		if (response.ok){
			const js = await response.json();
			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': Config received'
			document.getElementById('output').value = JSON.stringify(js,null,2)
			
			setConfigJson(js);
		 } else {
				alert(response.status);
		 }
	} catch(e) {
		alert(e.message);
	}    
}

var set_config = document.getElementById('set_config')
set_config.onclick = async (event) => {
	const settings = {
		method: 'POST',
		headers: {
			'Accept': 'application/json',
			'Content-Type': 'application/json',
		},
		body: JSON.stringify(getConfigJson())
	};
	
	try {
		const response = await fetch('/api/config/set', settings);
		
		if (response.ok){
			// const js = await response.json();
			const txt = await response.text();
			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': Config submitted';
			document.getElementById('output').value = txt;
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}    
}

var restart = document.getElementById('restart')
restart.onclick = async (event) => {
	try {
		const response = await fetch('/restart');
		if (response.ok){
			const txt = await response.text();
			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': Restart';
			document.getElementById('output').value = txt;
		} else {
			alert(response.status);
		}
	} catch(e) {
		alert(e.message);
	}    
}

async function getFileList() {
	try {
		const response = await fetch('/api/files/list');
		if (response.ok){
			const js = await response.json();

			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': Filelist received';
			document.getElementById('output').value = JSON.stringify(js);
			
			document.getElementById('filelist').value = '';
			addTable(js.files);

		 } else {
				alert(response.status);
		 }
	} catch(e) {
		alert(e.message);
	}    
}

async function removeFile(filename) {
	var formData = new FormData();
	formData.append('filename', filename);
	
	const settings = {
		method: 'POST',
		body: formData
	};
	
	try {
		const response = await fetch('/api/files/remove', settings);
		if (response.ok){
			const txt = await response.text();
			
			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': File removed';
			document.getElementById('output').value = txt;
			
			getFileList(); // refresh the file list on the page
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}  
}

// https://researchhubs.com/post/computing/javascript/upload-files-with-ajax.html		
var upload = document.getElementById('upload')
upload.onchange = async (event) => {
	
	var formData = new FormData(); // Create a new FormData object.
	
	if(event.target.files.length == 0) return;
	
	// Loop through each of the selected files.
	for (const file of event.target.files) {

	 // Check the file type.
	 // if (!file.type.match('image.*')) {
		 // continue;
	// }

		formData.append('files[]', file, file.name); // Add the file to the request.
	}

	const settings = {
		method: 'POST',
		body: formData
	};
	
	try {
		const response = await fetch('api/files/upload', settings);
		if (response.ok){
			const js = await response.json();
			
			const date = new Date();
			document.getElementById('status').innerHTML = date.toTimeString() + ': File uploaded!';
			document.getElementById('output').value = JSON.stringify(js);
			
			getFileList(); // refresh the file list on the page
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}  
}

function addTable(flist) {
  var myTableDiv = document.getElementById("filelist");
	
	myTableDiv.innerHTML = '';

  var table = document.createElement('TABLE');
  table.id = 'tbl_filelist';

  // var tableBody = document.createElement('TBODY');
  // table.appendChild(tableBody);

	// header
	var tr = document.createElement('TR');
	table.appendChild(tr);
	
	var width = ['300', '100']
	var header = ['Files', 'Size']
	for (var j = 0; j < 2; j++) {
		var th = document.createElement('TH');
		th.width = width[j];
		th.appendChild(document.createTextNode(header[j]));
		tr.appendChild(th);
	}
	
  for (var i=0; i<flist.length;i++){
    var tr = document.createElement('TR');
    table.appendChild(tr);
		
		var filename = flist[i]
		var td = document.createElement('TD');
		td.width = width[0];
		td.appendChild(document.createTextNode(filename));
		tr.appendChild(td);

		td = document.createElement('TD');
		td.width = width[1];
		
		var button = document.createElement("button");
		button.innerHTML = '-'
		button.name = filename
		button.onclick = function(){
			removeFile(this.name);
		};
		
		td.appendChild(button);
		tr.appendChild(td);
  }
  myTableDiv.appendChild(table);
}
		
//var filelist = document.getElementById('filelist')
//filelist.onclick = getFileList
document.body.onload = function() {
	getFileList()
	getConfig();
	document.getElementById('tab_config').click();
};