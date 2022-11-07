# PRO_HP-Box which is made from esp32 wroom, check the state of the humidifier and send data to server with LoraWan
<p><strong>Function:</strong><br>
Check the state of the humidifier depend on current and it consists of three states: good, error, not ready<br>
Check water level if water leaks, relay will turn off and relay will turn on when water doesnt leak<br>
It will automatically check very 5 seconds and send data server with LoraWan<br>
Sending data from server to mcu to turn on/off relay or set interval time</p>
