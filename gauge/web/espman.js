	// getElementById
	function $id(id) {
		return document.getElementById(id);
	}
	
		// output information
	function Output(msg) {
		const date = new Date();
		const m = $id("output");
		m.innerHTML = date.toTimeString() + "<br>" + msg + m.innerHTML;
	}
	
//---------------------------------------------------------------------------
// display/hide tab
function openTab(evt, tabName) {

  // Get all elements with class="tabcontent" and hide them
  const tabcontent = document.getElementsByClassName("tabcontent");
  for (let i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tablinks" and remove the class "active"
  const tablinks = document.getElementsByClassName("tablinks");
  for (let i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab, and add an "active" class to the button that opened the tab
  $id(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}

//---------------------------------------------------------------------------
// create form

function updateSensorCount(inc)
{
  const counter = $id('nsensors');
  const n = parseInt(counter.value); //innerHTML
  counter.value = n + inc;
}

$id('add_sensor').onclick = function(){ addSensor(); }

// add a sensor
function addSensor(sensor)
{
  const counter = $id('nsensors');
  const n = parseInt(counter.value); //innerHTML
  
  // snesor name
  const sensorId = "sensor[" + n + "]";
  
  const div = document.createElement("div");
  div.id = sensorId;
  div.className = 'sensor'
  
  // add '-' button
  const btn = document.createElement("input");
  btn.type = "button";
  btn.value = "-";
  btn.onclick = function()
  {
    $id("sensors").removeChild($id(sensorId));
    updateSensorCount(-1);
  }
  div.appendChild(btn);
  
  //const nameName = sensorId+"[name]";
  const lblName = document.createElement("label");
  //lblName.htmlFor = nameName;
  lblName.innerHTML = "Name:";
  const inName = document.createElement("input");
  inName.name = "name";
  inName.type = "text";
  inName.size = "5";
	inName.required = true;
	if(sensor) inName.value = sensor.name;
  
  // sensor type
  const typeName = sensorId + "[type]";
  const lblType = document.createElement("label");
  //lblType.htmlFor = typeName;
  lblType.innerHTML = "Type:"
  const selType = document.createElement("select");
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
    const option = document.createElement("option");
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
  const pins = "pins[" + n + "]";
  const div_pins = document.createElement("div");
  div_pins.id = pins;
	div_pins.name = "pins";

  // sensor type change event, create corresponding pins
  selType.onchange = function(){
    //alert(selType.value);
    
    // clear pins div
    div_pins.innerHTML = '';
  
    // reset pins div per sensor type
    const vsensor = sensor_types.get(selType.value);
    if(vsensor.length > 1)
    {
			// having pins definition
      for (const pinName of vsensor[1])
      {
        // create pin label
        //const pinName = sensorId + "[pin][" + i + "]";
        const lblPin = document.createElement("label");
        lblPin.htmlFor = pinName;
        lblPin.innerHTML = pinName + ':';
        
        // create pin select options
        const selPin = document.createElement("select");
        selPin.name = pinName;
        for (let [key, value] of pin_values)
        {
          const option = document.createElement("option");
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
  $id("sensors").appendChild(div);
  updateSensorCount(1);
	
	if(sensor)
	{
		selType.value = sensor.type;
		selType.onchange();
	}

/*  
  for (let i = 0; i < 3; i++)
  {
    const pinName = sensor + "[pin][" + i + "]";
    const lblPin = document.createElement("label");
    lblPin.htmlFor = pinName;
    lblPin.innerHTML = "Pin:"
    const selPin = document.createElement("select");
    selPin.name = "pin"; //pinName;
    for (let [key, value] of pin_values)
    {
      const option = document.createElement("option");
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
	let data = {};
	data.module = $id('module').value;
	
	data.wifi={'ssid': $id('ssid').value};
	data.wifi.pass = $id('pass').value;
	
	data.ip = $id('ip').value;
	data.gateway = $id('gateway').value;
	data.appass = $id('apPass').value;
	data.otapass = $id('otaPass').value;
	
	data.mqtt = {'server': $id('mqttServer').value};
	data.mqtt.port = Number($id('mqttPort').value);
	data.mqtt.user = $id('mqttUser').value;
	data.mqtt.pass = $id('mqttPass').value;
	
	data.sensors = [];
	
  const sensors = $id("sensors");
  for(let sitm of sensors.childNodes)
	{
		let sensor = {};
		for(let itm of sitm.childNodes)
		{
      if (itm.name == 'name') sensor.name = itm.value;
      else if (itm.name == 'type') sensor.type = itm.value;
      else if (itm.name == 'pins')
      {
				let pins = {};
				let hasPin = false;
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
	$id('module').value = js.module;
	
	$id('ssid').value 		= js.wifi.ssid;
	$id('pass').value 		= js.wifi.pass;
	$id('ip').value 		 	= js.ip;
	$id('gateway').value 	= js.gateway;
	$id('apPass').value 	= js.appass;
	$id('otaPass').value 	= js.otapass;
	
	$id('mqttServer').value = js.mqtt.server;
	$id('mqttPort').value 	= js.mqtt.port;
	$id('mqttUser').value 	= js.mqtt.user;
	$id('mqttPass').value 	= js.mqtt.pass;
	
	// reset sensors
  const sensors = $id("sensors");
	sensors.innerHTML = '';
	
	// reset counter
	const counter = $id('nsensors');
  counter.value = 0;
	
	for(const sensor of js.sensors)
	{
		addSensor(sensor);
	}
}

//---------------------------------------------------------------------------
// AJAX requests

// real-time update the configuration in config editor
$id('config').onchange = async (event) => {
	if(event.target.id === 'config_edit') return;

	const js = getConfigJson();
	$id('config_edit').value = JSON.stringify(js,null,2);
	// Output(
		// "<b>Get config json:</b><br><pre>" + JSON.stringify(js,null,2) + "</pre>"
	// );
}

$id('set_config_json').onclick = async (event) => {
	const js = JSON.parse($id('config_edit').value);
	setConfigJson(js);
}

async function getConfig() {
	try {
		const response = await fetch('/api/config/get');
		if (response.ok){
			const txt = await response.text(); // original text
			// const js = await response.json();
			const js = JSON.parse(txt);
			
			$id('config_edit').value = JSON.stringify(js,null,2);
			
			Output(
				"<b>Get config:</b><pre>" + txt + "</pre>"
			);
			
			setConfigJson(js);
		 } else {
				alert(response.status);
		 }
	} catch(e) {
		alert(e.message);
	}    
}

$id('get_config').onclick = async (event) => {
	getConfig();
}

$id('set_config').onclick = async (event) => {
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
			
			Output(
				"<b>Set config:</b><pre>" + txt + "</pre>"
			);

			// const date = new Date();
			// $id('status').innerHTML = date.toTimeString() + ': Config submitted';
			// $id('output').value = txt;
			
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}    
}

$id('restart').onclick = async (event) => {
	try {
		const response = await fetch('/restart');
		if (response.ok){
			const txt = await response.text();
			// const date = new Date();
			// $id('status').innerHTML = date.toTimeString() + ': Restart';
			// $id('output').value = txt;
			
			Output(
				"<b>Restart:</b><pre>" + txt + "</pre>"
			);
			
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

			// const date = new Date();
			// $id('status').innerHTML = date.toTimeString() + ': Filelist received';
			// $id('output').value = JSON.stringify(js);

			Output(
				"<b>Get filelist:</b><pre>" + JSON.stringify(js) + "</pre>"
			)
			
			$id('filelist').value = '';
			$id('diskinfo').innerHTML = 'Used:' + js.used.toLocaleString() + ' (' + js.used/js.max*100 +'%), Max:' + js.max.toLocaleString();
			addTable(js.files);
			
		 } else {
				alert(response.status);
		 }
	} catch(e) {
		alert(e.message);
	}    
}

async function removeFile(filename) {
	const formData = new FormData();
	formData.append('filename', filename);
	
	const settings = {
		method: 'POST',
		body: formData
	};
	
	try {
		const response = await fetch('/api/files/remove', settings);
		if (response.ok){
			const txt = await response.text();
			
			// const date = new Date();
			// $id('status').innerHTML = date.toTimeString() + ': File removed';
			// $id('output').value = txt;

			Output(
				"<b>File removed:</b><pre>" + filename + "</pre>"
			)
			
			getFileList(); // refresh the file list on the page
			
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}  
}

/*
// https://researchhubs.com/post/computing/javascript/upload-files-with-ajax.html		
$id('upload').onchange = async (event) => {
	
	const formData = new FormData(); // Create a new FormData object.
	
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
			$id('status').innerHTML = date.toTimeString() + ': File uploaded!';
			$id('output').value = JSON.stringify(js);
			
			getFileList(); // refresh the file list on the page
		} else {
			alert(response.status);
		}
		
	} catch(e) {
		alert(e.message);
	}  
}
*/

function addTable(flist) {
  const myTableDiv = $id("filelist");
	
	myTableDiv.innerHTML = '';

  const table = document.createElement('TABLE');
  table.id = 'tbl_filelist';

  // const tableBody = document.createElement('TBODY');
  // table.appendChild(tableBody);

	// header
	const tr = document.createElement('TR');
	table.appendChild(tr);
	
	// const width = ['300', '100']
	const header = ['Files', 'Size']
	for (let j = 0; j < 2; j++) {
		const th = document.createElement('TH');
		// th.width = width[j];
		th.appendChild(document.createTextNode(header[j]));
		tr.appendChild(th);
	}
	
  for (let i=0; i<flist.length;i++){
    const tr = document.createElement('TR');
    table.appendChild(tr);
		
		const file = flist[i]
		let td = document.createElement('TD');
		// td.width = width[0];

		const button = document.createElement("button");
		button.innerHTML = '-'
		button.name = file.name
		button.onclick = function(){
			removeFile(this.name);
		};
		td.appendChild(button);
		td.append(' '); // add space between button and file name
		td.appendChild(document.createTextNode(file.name));
		tr.appendChild(td);

		td = document.createElement('TD');
		td.appendChild(document.createTextNode(file.size.toLocaleString()));
		// td.width = width[1];

		tr.appendChild(td);
  }
  myTableDiv.appendChild(table);
}

//--------------------------------------------------
//const filelist = $id('filelist')
//filelist.onclick = getFileList

$id('clear').onclick = async (event) => {
		$id("output").innerHTML = '';
}

document.body.onload = function() {
	getFileList()
	getConfig();
	$id('tab_config').click();
};