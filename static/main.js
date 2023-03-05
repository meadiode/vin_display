

var socket = null;


function init() {    

    socket = new WebSocket('ws://' + window.location.host + '/ws');

    socket.addEventListener('open', function(e) {
        
        socket.addEventListener('close', function(e) {
            overlay = document.getElementById('offline_overlay');
            overlay.style.display = 'block';
        });

        socket.addEventListener('message', function(e) {
            dispatch_message(JSON.parse(e.data));
        });

    });

    var btn_power = document.getElementById('btn_power');
    
    btn_power.addEventListener('mousedown', function(e) {
        var req = {btn_power_press: true};
        socket.send(JSON.stringify(req));
    });

    btn_power.addEventListener('mouseup', function(e) {
        var req = {btn_power_press: false};
        socket.send(JSON.stringify(req));
    });

    btn_power.addEventListener('mouseleave', function(e) {
        var req = {btn_power_press: false};
        socket.send(JSON.stringify(req));
    });


    var btn_reset = document.getElementById('btn_reset');
    
    btn_reset.addEventListener('mousedown', function(e) {
        var req = {btn_reset_press: true};
        socket.send(JSON.stringify(req));
    });

    btn_reset.addEventListener('mouseup', function(e) {
        var req = {btn_reset_press: false};
        socket.send(JSON.stringify(req));
    });

    btn_reset.addEventListener('mouseleave', function(e) {
        var req = {btn_reset_press: false};
        socket.send(JSON.stringify(req));
    });


    document.getElementById('default_tab').click();
    console.log('init done');
}

function format_uptime(uptime_seconds) {
    var uptime = Math.floor(uptime_seconds);
    var seconds = l02(uptime % 60);
    var minutes = Math.floor(uptime / 60) % 60;
    var hours = Math.floor(uptime / (60 * 60)) % 24;
    var days = Math.floor(uptime / (60 * 60 * 24));

    function l02(val) {
        return ('00' + String(val)).slice(-2);
    }

    var res = `${l02(days)}d ${l02(hours)}:${l02(minutes)}:${l02(seconds)}`
    res = 'Uptime: ' + res;

    return res;
}

function format_temperature(temp_c) {
    var temp = temp_c.toFixed(1);
    var res = ('  ' + String(temp) + 'C').slice(-7);
    res = 'Temperature: ' + res;
    return res;
}

function dispatch_message(msg) {

    if ('power_on' in msg) {
        var btn_power = document.getElementById('btn_power');
        var led_power = document.getElementById('led_power');

        if (msg.power_on == true)
        {
            btn_power.style.backgroundImage = 'url("/power_btn_on.svg")';
            led_power.style.backgroundImage = 'url("/led_green.svg")';
        } else {
            btn_power.style.backgroundImage = 'url("/power_btn_off.svg")';
            led_power.style.backgroundImage = 'url("/led.svg")';
        }
    }


    if ('hdd_on' in msg) {
        var led_hdd = document.getElementById('led_hdd');

        if (msg.hdd_on == true)
        {
            led_hdd.style.backgroundImage = 'url("/led_yellow.svg")';
        } else {
            led_hdd.style.backgroundImage = 'url("/led.svg")';
        }
    }


    if ('pc_uptime' in msg) {
        var info_uptime = document.getElementById('info_uptime');
        info_uptime.innerHTML = format_uptime(msg['pc_uptime']);
    }

    if ('temp' in msg) {
        var info_temp = document.getElementById('info_temp');
        info_temp.innerHTML = format_temperature(msg['temp']);
    }
}


function switch_tab(evt, tab_id) {
    var i, tabcontent, tablinks;

    /* Get all elements with class="tabcontent" and hide them */
    tabcontent = document.getElementsByClassName("tabcontent");

    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    /* Get all elements with class="tablinks" and remove the class "active" */
    tablinks = document.getElementsByClassName("tablinks");
    
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }

    /* Show the current tab, and add an "active" class to the button that opened the tab */
    document.getElementById(tab_id).style.display = "block";
    evt.currentTarget.className += " active";
}