#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawcanvas(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: sans-serif; text-align: center; background-color: #222; color: #fff; user-select: none; }
        .row { display: flex; justify-content: center; align-items: center; gap: 10px; width: 95%; margin: 5px auto; }
        .btn { flex: 1; padding: 15px; font-size: 18px; border: none; border-radius: 10px; cursor: pointer; font-weight: bold; }
        .arm { background-color: #4CAF50; color: white; }
        .disarm { background-color: #f44336; color: white; }
        .vision-btn { background-color: #444; color: white; border: 1px solid #777; }
        .capture { background-color: #cb36f4; color: white; width: 90%; margin: 10px auto; display: block; }
        input[type=range] { width: 90%; height: 50px; margin: 20px 0; }
        #snitch-canvas { width:320px; height:240px; border:2px solid #555; background-color: #000; display: block; margin: 10px auto; image-rendering: pixelated; }
    </style>
</head>
<body>
    <h1>SNITCH CONTROL</h1>

    <h2 id='status' style='color:#4CAF50;'>STANDBY</h2>
    <div class='row'>
        <button class='btn arm' onclick='setArm(true)'>ARM</button>
        <button class='btn disarm' onclick='setArm(false)'>DISARM</button>
    </div>

    <div class='row' style='margin-top:20px;'>
        <h3 id='vision' style='margin:0; color:#f44336;'>AUTO: OFF</h3>
        <button class='btn vision-btn' onclick='toggleVision()'>TOGGLE AUTO</button>
    </div>

    <h3 id='thrVal'>Throttle: 0%</h3>
    <input type='range' min='1000' max='2000' value='1000' id='slider' oninput='sendThrottle(this.value)'>

    <button class='btn capture' id='capture-btn' onclick='toggleStream()'>TOGGLE STREAM</button>
    <canvas id='snitch-canvas' width='160' height='120'></canvas>

    <script>
        var streamActive = false;
        const canvas = document.getElementById('snitch-canvas');
        const ctx = canvas.getContext('2d');
        const imageData = ctx.createImageData(160, 120);

        function toggleStream() {
            streamActive = !streamActive;
            const btn = document.getElementById('capture-btn');
            
            if (streamActive) {
                btn.innerText = "STOP STREAM";
                btn.style.backgroundColor = "#444";
                updateFrame();
            } else {
                btn.innerText = "START STREAM";
                btn.style.backgroundColor = "#cb36f4";
                // Delay the clear slightly to catch any "in-flight" frames
                setTimeout(() => {
                    ctx.fillStyle = "black";
                    ctx.fillRect(0, 0, canvas.width, canvas.height);
                }, 100);
            }
        }

        async function updateFrame() {
            if (!streamActive) return;
            try {
                const response = await fetch('/capture');
                const buffer = await response.arrayBuffer();
                const pixels = new Uint8Array(buffer);
                
                for (let i = 0; i < pixels.length; i++) {
                    const p = pixels[i];
                    const off = i * 4;
                    imageData.data[off] = p;     // R
                    imageData.data[off+1] = p;   // G
                    imageData.data[off+2] = p;   // B
                    imageData.data[off+3] = 255; // Alpha
                }
                
                ctx.putImageData(imageData, 0, 0);
                requestAnimationFrame(updateFrame);
            } catch (e) {
                console.error(e);
                setTimeout(updateFrame, 500);
            }
        }

        function setArm(arm) {
            var statusText = document.getElementById('status');
            statusText.innerText = arm ? 'ARMED!' : 'STANDBY';
            statusText.style.color = arm ? '#f44336' : '#4CAF50';
            
            // Safety: Force slider to 1000 on arming to avoid ESC lockout
            if(arm) {
                document.getElementById('slider').value = 1000;
                sendThrottle(1000);
            }
            
            fetch(arm ? '/arm' : '/disarm');
            
            if (!arm) {
                document.getElementById('slider').value = 1000;
                sendThrottle(1000);
            }
        }

        function sendThrottle(val) {
            document.getElementById('thrVal').innerText = 'Throttle: ' + Math.round((val-1000)/10) + '%';
            fetch('/throttle?val=' + val);
        }

        function toggleVision() {
            var currVal = document.getElementById('vision');
            if (currVal.innerText === 'AUTO: OFF') {
                currVal.innerText = 'AUTO: ON';
                currVal.style.color = '#4CAF50';
            } else {
                currVal.innerText = 'AUTO: OFF';
                currVal.style.color = '#f44336';
            }
            fetch('/vision');
        }
    </script>
</body>
</html>
)rawcanvas";

#endif