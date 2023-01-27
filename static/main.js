

var STATUS = {
    UNKNOWN         : 0,
    DISCONNECTED    : 1,
    CONNECTED       : 2,
    UNSTABLE        : 3
};

var con_history = [];
var con_history_pos = 0;
var con_history_tmp = '';

var con_log_scrolled = false;


function con_log_update(entry) {

    var conlog = document.getElementById('console_log');

    conlog.innerText = conlog.innerText + '\n' + entry;
    
    if (!con_log_scrolled) {
        var scroll_end = conlog.scrollHeight - conlog.clientHeight;
        conlog.scrollTop = Math.max(0, scroll_end);
    }
}


function set_status(status) {
    var dot = document.getElementById('status_dot');
    var stxt = document.getElementById('status_text');

    switch(status) {
    
    case STATUS.DISCONNECTED:
        dot.className = 'status_disconnected';
        stxt.innerText = 'Disconnected';
        break;

    case STATUS.CONNECTED:
        dot.className = 'status_connected';
        stxt.innerText = 'Connected';
        break;

    case STATUS.UNSTABLE:
        dot.className = 'status_unstable';
        stxt.innerText = 'Unstable connection';
        break;

    case STATUS.UNKNOWN:
    default:
        dot.className = 'status_unknown';
        stxt.innerText = 'Status unknown'
    }

}

function init() {

    var inp = document.getElementById('console_input');

    inp.addEventListener('keypress', function(evt) {
        
        /* enter key */
        if (evt.keyCode == 13) {
            cmd_req = inp.innerText;
            inp.innerText = '';
        
            if ((cmd_req !== '') && (cmd_req !== con_history.slice(-1)[0])) {
                con_history.push(cmd_req);
            }
            con_history_pos = 0;

            if (cmd_req !== '') {
                console.log('Sending cmd: ', cmd_req);

                var req = new XMLHttpRequest();
                req.open('POST', '/cmd', true);
                req.setRequestHeader('Accept', 'application/json');
                req.setRequestHeader('Content-Type', 'application/json');


                req.onload = function () {
                    if (req.readyState != 4 || req.status != 200) {
                        console.log('request failed');
                        console.log('status: ', req.status);
                    } else {
                        con_log_update(req.responseText);
                        set_status(STATUS.CONNECTED);
                    }
                    console.log(req.responseText);
                }

                var payload = {cmd: cmd_req};
                req.send(JSON.stringify(payload));                
            }

            evt.preventDefault();
        }
    });

    inp.addEventListener('keydown', function(evt) {
        /* Cycle through console history */
        if (con_history.length > 0) {
            
            /* up arrow key */
            if (evt.keyCode == 38) {

                if (con_history_pos == 0) {
                    con_history_tmp = inp.innerText;
                }

                con_history_pos -= 1;

            /* down arrow key */
            } else if (evt.keyCode == 40) {
                
                if (con_history_pos == 0) {
                    con_history_tmp = inp.innerText;
                    con_history_pos = -con_history.length;
                } else {
                    con_history_pos += 1;
                }
            };

            if (evt.keyCode == 38 || evt.keyCode == 40)
            {
                if (Math.abs(con_history_pos) > con_history.length) {
                    con_history_pos = 0;
                }

                if (con_history_pos == 0) {
                    inp.innerText = con_history_tmp;
                } else {
                    inp.innerText = con_history.slice(con_history_pos)[0];
                }
            }
        }
    });

    
    var conlog = document.getElementById('console_log');

    conlog.addEventListener('scroll', function(evt) {
        var scroll_end = conlog.scrollHeight - conlog.clientHeight;
        
        con_log_scrolled = !(conlog.scrollTop >= scroll_end);
    });

    console.log('init done');
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