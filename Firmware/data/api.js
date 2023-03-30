
document.getElementById("defaultOpen").click();
document.getElementById("exp-input").value = document.getElementById("exp-range").value = localStorage.getItem("exposure")||1;
document.getElementById("r-gain-input").value = document.getElementById("r-gain-range").value = localStorage.getItem("r-gain")||1;
document.getElementById("g-gain-input").value = document.getElementById("g-gain-range").value = localStorage.getItem("g-gain")||1;
document.getElementById("b-gain-input").value = document.getElementById("b-gain-range").value = localStorage.getItem("b-gain")||1;
document.getElementById("global-gain-input").value = document.getElementById("global-gain-range").value = localStorage.getItem("global-gain")||1;
document.getElementById("interval-input").value = document.getElementById("interval-range").value = localStorage.getItem("interval")||1;
document.getElementById("frames-input").value = document.getElementById("frames-range").value = localStorage.getItem("frames")||1;
document.getElementById("progress-input").value = " 0 / " + document.getElementById("frames-input").value;
document.getElementById("video-length-input").value = (document.getElementById("frames-input").value / 30).toFixed(0);
document.getElementById("exp-input").oninput = document.getElementById("exp-range").oninput = function () {
    document.getElementById("exp-input").value = this.value;
    document.getElementById("exp-range").value = this.value;
    localStorage.setItem("exposure", this.value);
}
document.getElementById("r-gain-input").oninput = document.getElementById("r-gain-range").oninput = function () {
    document.getElementById("r-gain-input").value = this.value;
    document.getElementById("r-gain-range").value = this.value;
    localStorage.setItem("r-gain", this.value);
}
document.getElementById("g-gain-input").oninput = document.getElementById("g-gain-range").oninput = function () {
    document.getElementById("g-gain-input").value = this.value;
    document.getElementById("g-gain-range").value = this.value;
    localStorage.setItem("g-gain", this.value);
}
document.getElementById("b-gain-input").oninput = document.getElementById("b-gain-range").oninput = function () {
    document.getElementById("b-gain-input").value = this.value;
    document.getElementById("b-gain-range").value = this.value;
    localStorage.setItem("b-gain", this.value);
}
document.getElementById("global-gain-input").oninput = document.getElementById("global-gain-range").oninput = function () {
    document.getElementById("global-gain-input").value = this.value;
    document.getElementById("global-gain-range").value = this.value;
    localStorage.setItem("global-gain", this.value);
}

document.getElementById("interval-input").oninput = document.getElementById("interval-range").oninput = function () {
    document.getElementById("interval-input").value = this.value;
    document.getElementById("interval-range").value = this.value;
    localStorage.setItem("interval", this.value);
}
document.getElementById("frames-input").oninput = document.getElementById("frames-range").oninput = function () {
    document.getElementById("frames-input").value = this.value;
    document.getElementById("frames-range").value = this.value;
    localStorage.setItem("frames", this.value);

    document.getElementById("progress-input").value = " 0 / " + document.getElementById("frames-input").value;
    document.getElementById("video-length-input").value = (document.getElementById("frames-input").value / 30).toFixed(0);
}
$(function () {
    window.Translator.translate(localStorage.getItem("lang"));
    document.getElementById("lang-select").value = localStorage.getItem("lang")||"english";
});
document.getElementById("lang-select").oninput = function()
{
    window.Translator.translate(this.value);
    
    $(function () {
        window.Translator.translate(this.value);
    });
    localStorage.setItem("lang", this.value);
}

document.getElementById("check-udisk").checked = localStorage.getItem("udisk")||false;
document.getElementById("check-udisk").oninput = function()
{
    if(this.checked)
    {
        request(`/storage/set?mode=udisk`).then(res => {
        });
    }
    else{
        request(`/storage/set?mode=camera`).then(res => {
        });
    }
    
    localStorage.setItem("udisk", this.value);
}
function openPage(evt, pageName) {
    let i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }
    document.getElementById(pageName).style.display = "block";
    evt.currentTarget.className += " active";
}
let waitTime = 1000;
function request(url) {
    return new Promise(resolve => {
        document.getElementById('appLoading').style.display = 'block';
        let oReq = new XMLHttpRequest();
        oReq.addEventListener("load", function () {
            document.getElementById('appLoading').style.display = 'none'
            resolve(this.responseText);
        });
        oReq.open("GET", url);
        oReq.send();
    });
}
async function onShot() {
    document.getElementById('appLoading').style.display = 'block';
    await onExposure();
    request('/capture/shot').then(res => {
        let obj = JSON.parse(res);
        setTimeout(() => {
            document.getElementById('appLoading').style.display = 'none'
        }, obj.time * 1.0 + waitTime);
    });
}
function onReload() {
    document.getElementById('preview-img').src = '/preview/latest.jpg?time=' + Date.now();
}
function onExposure() {
    return new Promise(resolve => {
        let exp = document.getElementById('exp-input').value * 1000;
        let r_gain = document.getElementById('r-gain-input').value;
        let g_gain = document.getElementById('g-gain-input').value;
        let b_gain = document.getElementById('b-gain-input').value;
        let gain = document.getElementById('global-gain-input').value;
        request(`/capture/set?coarse=${exp}&fine=${1}&gain=${gain}&r_gain=${r_gain}&gr_gain=${g_gain}&gb_gain=${g_gain}&b_gain=${b_gain}`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}
function onAddTask() {
    return new Promise(resolve => {
        let exp = document.getElementById('exp-input').value * 1000;
        let r_gain = document.getElementById('r-gain-input').value;
        let g_gain = document.getElementById('g-gain-input').value;
        let b_gain = document.getElementById('b-gain-input').value;
        let interval = document.getElementById("interval-input").value;
        let frames = document.getElementById("frames-input").value;
        request(`/task/add?delay=100&during=${interval}&frames=${frames}&mode=3&resolution=0&coarse=${exp}&fine=${1}&r_gain=${r_gain}&gr_gain=${g_gain}&gb_gain=${g_gain}&b_gain=${b_gain}`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}
async function onStartTask() {
    await syncTime();
    await onAddTask();
    return new Promise(resolve => {
        request(`/task/start`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}
function onStopTask() {
    return new Promise(resolve => {
        request(`/task/stop`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}
getTaskStatus();
function getTaskStatus() {
    return new Promise(resolve => {
        request(`/task/status`).then(res => {
            let obj = JSON.parse(res);
            if(obj&&obj.total)
            {
                document.getElementById("progress-input").value = ` ${obj.current} / ${obj.total}`;
            }
            else
            {
                document.getElementById("progress-input").value = " 0 / " + document.getElementById("frames-input").value;
            }
            resolve();
        });
    })
}

function syncTime() {
    let now = new Date();
    return new Promise(resolve => {
        request(`/time/set?time=${(now.getTime()/1000-now.getTimezoneOffset()*60)>>0}`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}
function getDirectoryList(path) {
    return new Promise(resolve => {
        request(`/${path}`).then(res => {
            resolve(JSON.parse(res))
        });
    })
}