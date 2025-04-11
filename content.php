<?
$LoRaSpeeds = Array();
$LoRaSpeeds['1200'] = '1200';
$LoRaSpeeds['2400'] = '2400';
$LoRaSpeeds['4800'] = '2400';
$LoRaSpeeds['9600'] = '9600';
$LoRaSpeeds['19200'] = '19200';
$LoRaSpeeds['38400'] = '38400';
$LoRaSpeeds['57600'] = '57600';
$LoRaSpeeds['115200'] = '115200';

$LoRaAirSpeeds = Array();
$LoRaAirSpeeds['300'] = '300';
$LoRaAirSpeeds['1200'] = '1200';
$LoRaAirSpeeds['2400'] = '2400';
$LoRaAirSpeeds['4800'] = '4800';
$LoRaAirSpeeds['9600'] = '9600';
$LoRaAirSpeeds['15600'] = '15600';
$LoRaAirSpeeds['19200'] = '19200';
$LoRaAirSpeeds['38400'] = '38400';
$LoRaAirSpeeds['62500'] = '62500';

$LoRaAirSpeeds1276 = Array();

$LoRaTransmitPower = Array();
$LoRaTransmitPower['High'] = '4';
$LoRaTransmitPower['Medium'] = '3';
$LoRaTransmitPower['Low'] = '1';
$LoRaTransmitPower['Very Low'] = '1';


$LoRaAdapterTypes = Array();
$LoRaAdapterTypes['E32-433T20D'] = 'E32-433T20D';
$LoRaAdapterTypes['E32-915T30D'] = 'E32-915T30D';
$LoRaAdapterTypes['E22-230T22U'] = 'E22-230T22U';
$LoRaAdapterTypes['E22-400T22U'] = 'E22-400T22U';
$LoRaAdapterTypes['E22-900T22U'] = 'E22-900T22U';
$LoRaAdapterTypes['SX1262'] = 'SX1262';
$LoRaAdapterTypes['SX1268'] = 'SX1268';
$LoRaAdapterTypes['SX1276'] = 'SX1276';
$LoRaAdapterTypes['SX1278'] = 'SX1278';

$LoRaTransmitChannel = Array();

$LoRaPorts = Array();
foreach(scandir("/dev/") as $fileName) {
    if ((preg_match("/^ttyS[0-9]+/", $fileName)) ||
        (preg_match("/^ttyACM[0-9]+/", $fileName)) ||
        (preg_match("/^ttyO[0-9]/", $fileName)) ||
        (preg_match("/^ttyS[0-9]/", $fileName)) ||
        (preg_match("/^ttyAMA[0-9]+/", $fileName)) ||
        (preg_match("/^ttyUSB[0-9]+/", $fileName))) {
        $LoRaPorts[$fileName] = $fileName;
    }
}
?>

<script type="text/javascript">
var LoRaAirSpeeds1262 = Array(); //900
LoRaAirSpeeds1262['2400'] = '2400';
LoRaAirSpeeds1262['4800'] = '4800';
LoRaAirSpeeds1262['9600'] = '9600';
LoRaAirSpeeds1262['19200'] = '19200';
LoRaAirSpeeds1262['38400'] = '38400';
LoRaAirSpeeds1262['62500'] = '62500';


var LoRaAirSpeeds1276 = Array();
LoRaAirSpeeds1276['300'] = '300';
LoRaAirSpeeds1276['1200'] = '1200';
LoRaAirSpeeds1276['2400'] = '2400';
LoRaAirSpeeds1276['4800'] = '4800';
LoRaAirSpeeds1276['9600'] = '9600';
LoRaAirSpeeds1276['19200'] = '19200';

var LoRaAirSpeeds1278 = Array();
LoRaAirSpeeds1278['300'] = '300';
LoRaAirSpeeds1278['1200'] = '1200';
LoRaAirSpeeds1278['2400'] = '2400';
LoRaAirSpeeds1278['4800'] = '4800';
LoRaAirSpeeds1278['9600'] = '9600';
LoRaAirSpeeds1278['19200'] = '19200';

var LoRaAirSpeeds1268 = Array(); //400
LoRaAirSpeeds1268['2400'] = '2400';
LoRaAirSpeeds1268['4800'] = '4800';
LoRaAirSpeeds1268['9600'] = '9600';
LoRaAirSpeeds1268['19200'] = '19200';
LoRaAirSpeeds1268['38400'] = '38400';
LoRaAirSpeeds1268['62500'] = '62500';

var LoRaAirSpeeds1262 = Array(); //900
LoRaAirSpeeds1262['2400'] = '2400';
LoRaAirSpeeds1262['4800'] = '4800';
LoRaAirSpeeds1262['9600'] = '9600';
LoRaAirSpeeds1262['19200'] = '19200';
LoRaAirSpeeds1262['38400'] = '38400';
LoRaAirSpeeds1262['62500'] = '62500';

var LoRaAirSpeeds1262_230 = Array(); //230
LoRaAirSpeeds1262_230['2400'] = '2400';
LoRaAirSpeeds1262_230['4800'] = '4800';
LoRaAirSpeeds1262_230['9600'] = '9600';
LoRaAirSpeeds1262_230['15600'] = '15600';


function setupChannels(start, increment, max, defaultChannel) {
    var ch = document.getElementById("CH");
    var originalChannel = pluginSettings['CH'];
    var found = false;
    ch.options.length = 0;
    for (var i = 0; i <= max; i++) {
        var opt = document.createElement("option");
        opt.value = start + (increment * i);
        opt.innerHTML = start + (increment * i);
        if (opt.value == originalChannel) {
            found = true;
        }
        ch.appendChild(opt);
    }
    if (!found) {
        originalChannel = defaultChannel;
    }
    ch.value = originalChannel;
    if (!found) {
        CHChanged();
    }
}

function SetupDefaultsForModule() {
    var moduleType = document.getElementById("LoRaDeviceType").value;
    var airspeeds = LoRaAirSpeeds1268;
    if (moduleType == "SX1276") {
        airspeeds = LoRaAirSpeeds1276;
        setupChannels(900, 1, 0x1F, 915);
    } else if (moduleType == "SX1278") {
        airspeeds = LoRaAirSpeeds1278;
        setupChannels(410, 1, 0x1F, 433);
    } else if (moduleType == "SX1262") {
        airspeeds = LoRaAirSpeeds1262;
        setupChannels(850, 1, 0x51, 915);
    } else if (moduleType == "SX1268") {
        airspeeds = LoRaAirSpeeds1268;
        setupChannels(410, 1, 0x53, 433);
    } else if (moduleType == "E22-230T22U") {
        airspeeds = LoRaAirSpeeds1262_230;
        setupChannels(220, 0.25, 0x40, 230);
    } else if (moduleType == "E22-400T22U") {
        airspeeds = LoRaAirSpeeds1268;
        setupChannels(410, 1, 0x53, 433);
    } else if (moduleType == "E22-900T22U") {
        airspeeds = LoRaAirSpeeds1262;
        setupChannels(850, 1, 0x51, 915);
    } else if (moduleType == "E32-433T20D") {
        airspeeds = LoRaAirSpeeds1278;
        setupChannels(410, 1, 0x1F, 433);
    } else if (moduleType == "E32-915T30D") {
        airspeeds = LoRaAirSpeeds1276;
        setupChannels(900, 1, 0x1F, 915);
    }
    var orig = document.getElementById("ADR").value;
    var found = false;
    document.getElementById("ADR").options.length = 0;
    for (var i in airspeeds) {
        var opt = document.createElement("option");
        opt.value = i;
        opt.innerHTML = i;
        document.getElementById("ADR").appendChild(opt);
        if (i == orig) {
            found = true;
        }
    }
    if (!found) {
        orig = "2400";
    }
    document.getElementById("ADR").value = orig;
    if (!found) {
        ADRChanged();
    }
}

function submitLoRaConfig() {
    //<form  id="LoRaconfigForm" action="api/plugin-apis/LoRa" method="post">
    var config = {};
    config.LoRaDeviceType = document.getElementById("LoRaDeviceType").value;
    config.LoRaDevicePort = document.getElementById("LoRaDevicePort").value;
    config.MA = parseInt(document.getElementById("MA").value);
    config.UBR = parseInt(document.getElementById("UBR").value);
    config.ADR = parseInt(document.getElementById("ADR").value);
    config.FEC = parseInt(document.getElementById("FEC").checked ? 1 : 0);
    config.TXP = parseInt(document.getElementById("TXP").value);
    config.CH = parseFloat(document.getElementById("CH").value);


    fetch("api/plugin-apis/LoRa", {
        method: "POST",
        headers: {'Content-Type': 'application/json'}, 
        body: JSON.stringify(config)
    }).then(res => {
        console.log("Request complete! response:", res);
    });
}
</script>


<div id="global" class="settings">
<fieldset>
<legend>LoRa MultiSync Configuration</legend>

Enable LoRa Plugin:  <? PrintSettingCheckbox("Enable LoRa", "LoRaEnable", 1, 0, "1", "0", "fpp-LoRa"); ?>
<p>
<p>
The FPP LoRa plugin supports using the UART/USB based LoRa modules to send/receive MultiSync packets via LoRa wireless.   It currently
supports the EBYTE "E32-*" modules including the E32-915T30D, E32-433T20D, etc... and USB LoRa modules based on the SX1262 and SX1268 chips such as the EBYTE E22-900T22U 
<p>
For the UART based modules, they can be wired directly to the UART pins on the Pi/BBB or by pluging into the USB port using a E15-USB-T2 adapter (recommended).
Make sure the M1 and M2 pins are pulled low.  On the E15-USB-T2, make sure the two jumpers are in place.
<p>
These setting much match what the module has been configured to use.<br>
Port:  <? PrintSettingSelect("Port", "LoRaDevicePort", 1, 0, 'ttyUSB0', $LoRaPorts, "fpp-LoRa") ?>
&nbsp; Baud Rate: <? PrintSettingSelect("Speed", "LoRaDeviceSpeed", 1, 0, '9600', $LoRaSpeeds, "fpp-LoRa") ?>
<p>
    <? if ($settings['fppMode'] == "player") { ?>
Send media sync packets: <? PrintSettingCheckbox("Enable LoRa Media", "LoRaMediaEnable", 1, 0, "1", "0", "fpp-LoRa", "", "1");  ?>
<p>
The LoRa protocol is very slow, it defaults to 2400 baud over the air.   At the start of each sequence, we have to send the
filenames for the sequence and the media which can take significant time if the filenames are long.  If you know the remotes will not 
need the media filenames, turn off sending the media sync packets which will help the remotes start quicker.
<p>
    <? } else { ?>
Bridge to local network: <? PrintSettingCheckbox("Enable LoRa Bridge", "LoRaBridgeEnable", 1, 0, "1", "0", "fpp-LoRa"); ?>
<p>
Bridging to the local network will re-send the sync packets out on the local network so other FPP devices on the network can
be synced.   This remote must have it's MultiSync targets set while in Master mode once before being placed back
into Remote mode.
    <? } ?>
<p>

</fieldset>
<br>

<fieldset>
<legend>LoRa Device Configuration</legend>
In order to configure the LoRa device, FPPD must be running with the LoRa plugin loaded.
The module must be put into Sleep mode.  For USB based  modules, this is usually done by pressing and holding
a user button on the device for about 2 seconds.  A red light may come on.   For TTY based modules, the module
put into sleep mode by pulling M1 and M2 high.  If using the E15-USB-T2 adapter, just remove
the M1 and M2 jumpers.   
<p>
    <p>
Put the modules in sleep mode, configure the settings below, and then press the "Configure" button.  The module will
be configured.  You will then need to take the module out of sleep mode by re-installing the jumpers or repressing the
user button.
<p>

Module Type: <? PrintSettingSelect("LoRaDeviceType", "LoRaDeviceType", 0, 0, 'SX1262', $LoRaAdapterTypes, "fpp-LoRa", "SetupDefaultsForModule") ?>
<br>
Module Address (0-65535): <? PrintSettingTextSaved("MA", 0,0 , 65535, 0,  "fpp-LoRa" ,0 , '', '', 'number'); ?>
<br>
UART Baud Rate: <? PrintSettingSelect("UBR", "UBR", 0, 0, '9600', $LoRaSpeeds, "fpp-LoRa") ?>
<br>
Air Data Rate: <? PrintSettingSelect("ADR", "ADR", 0, 0, '2400', $LoRaAirSpeeds, "fpp-LoRa") ?>
<br>
FEC (Forward Error Correction): <? PrintSettingCheckbox("FEC", "FEC", 0, 0, 1, 0, "fpp-LoRa", '', 1) ?>
<br>
Transmit Power: <? PrintSettingSelect("TXP", "TXP", 0, 0, '4', $LoRaTransmitPower, "fpp-LoRa") ?>
<br>
Channel/Frequency: <? PrintSettingSelect("CH", "CH", 0, 0, '', $LoRaTransmitChannel, "fpp-LoRa") ?>
<br>
<input type="button" value="Configure" class="buttons" onclick="submitLoRaConfig()"/>



<script type='text/javascript'>
    SetupDefaultsForModule();
</script>
</fieldset>
