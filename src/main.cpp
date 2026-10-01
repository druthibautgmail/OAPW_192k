#include "httplib.h"
#include "RACEDspEngine.h"

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <cmath>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Normierungskonstante für 32-Bit Audio (S32_LE)
constexpr double NORM_32 = 2147483648.0;

const char* HTML_CONTENT = R"HTML(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OAPW Control</title>
    <style>
        body { background-color: #121212; color: #ffffff; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; padding: 2rem; max-width: 600px; margin: 0 auto; }
        h1 { font-weight: 300; border-bottom: 1px solid #333; padding-bottom: 10px; margin-bottom: 2rem; }
        .control-group { margin-bottom: 1.5rem; background: #1e1e1e; padding: 1.5rem; border-radius: 8px; }
        .calc-group { border: 1px solid #007aff; }
        .eq-band { border-top: 1px solid #333; padding-top: 1rem; margin-top: 1rem; }
        label { display: flex; justify-content: space-between; margin-bottom: 0.5rem; font-weight: bold; font-size: 0.9rem; }
        input[type="range"] { width: 100%; cursor: pointer; margin-bottom: 1rem; }
        input[type="number"] { width: 100%; padding: 10px; margin-bottom: 15px; background: #333; color: white; border: none; border-radius: 4px; box-sizing: border-box; font-size: 1rem; }
        .value-display { font-weight: normal; color: #aaa; }
        button { width: 100%; padding: 15px; font-size: 1.2rem; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; background: #007aff; color: white; margin-bottom: 1.0rem; transition: background 0.3s;}
        button.off { background: #333; color: #888; }
        h3 { margin-top: 0; margin-bottom: 1rem; font-weight: 500; color: #007aff; }
        h4 { margin-top: 0; margin-bottom: 1rem; color: #fff; font-size: 1rem; }
        canvas { width: 100%; height: 180px; background: #000; border-radius: 8px; margin-bottom: 1rem; display: block; border: 1px solid #333; }
    </style>
</head>
<body>
    <h1>OAPW Ambiophonics DSP (192kHz 64-Bit)</h1>
    
    <button id="btnRace" onclick="toggleRace()">RACE: AN</button>
    <button id="btnFilters" onclick="toggleFilters()">Filter: AN</button>
    <button id="btnEq" onclick="toggleEq()">EQ: AUS</button>

    <div class="control-group">
        <h3>Real-Time Analyzer [Hz]</h3>
        <canvas id="analyzer" width="600" height="180"></canvas>
    </div>

    <div class="control-group calc-group">
        <h3>Geometrie-Rechner</h3>
        <label>Abstand Lautsprecher (Mitte-Mitte in cm)</label>
        <input type="number" id="spkDist" value="30">
        <label>Abstand Hörer zur Basislinie (in cm)</label>
        <input type="number" id="listDist" value="80">
        <button onclick="calcGeometry()" style="margin-bottom: 0; background: #0a84ff;">Berechnen & Anwenden</button>
    </div>

    <div class="control-group">
        <h3>DSP Parameter</h3>
        <label>Lautstärke <span id="valVol" class="value-display">50%</span></label>
        <input type="range" id="vol" min="0" max="100" value="50" oninput="updateParam('vol', this.value)">

        <label>Interaural Delay <span id="valDelay" class="value-display">68 &micro;s</span></label>
        <input type="range" id="delay" min="20" max="1500" step="2" value="68" oninput="updateParam('delay', this.value)">

        <label>Attenuation <span id="valAtt" class="value-display">-2.3 dB</span></label>
        <input type="range" id="att" min="-30" max="0" step="0.1" value="-2.3" oninput="updateParam('att', this.value)">

        <label>Center Parameter <span id="valCenter" class="value-display">0.10</span></label>
        <input type="range" id="center" min="0" max="100" value="10" oninput="updateParam('center', this.value)">
    </div>

    <div class="control-group">
        <h3>Parametrischer Equalizer (PEQ)</h3>
        <div class="eq-band">
            <h4>Band 1 (Bass)</h4>
            <label>Frequenz <span id="valEq0F" class="value-display">100 Hz</span></label>
            <input type="range" id="eq0F" min="20" max="500" step="1" value="100" oninput="updateEq(0, 'f', this.value)">
            <label>Q-Faktor <span id="valEq0Q" class="value-display">0.7</span></label>
            <input type="range" id="eq0Q" min="0.1" max="5.0" step="0.1" value="0.7" oninput="updateEq(0, 'q', this.value)">
            <label>Gain <span id="valEq0G" class="value-display">0.0 dB</span></label>
            <input type="range" id="eq0G" min="-15" max="15" step="0.5" value="0" oninput="updateEq(0, 'g', this.value)">
        </div>
        <div class="eq-band">
            <h4>Band 2 (Mitten)</h4>
            <label>Frequenz <span id="valEq1F" class="value-display">1000 Hz</span></label>
            <input type="range" id="eq1F" min="200" max="5000" step="10" value="1000" oninput="updateEq(1, 'f', this.value)">
            <label>Q-Faktor <span id="valEq1Q" class="value-display">0.7</span></label>
            <input type="range" id="eq1Q" min="0.1" max="5.0" step="0.1" value="0.7" oninput="updateEq(1, 'q', this.value)">
            <label>Gain <span id="valEq1G" class="value-display">0.0 dB</span></label>
            <input type="range" id="eq1G" min="-15" max="15" step="0.5" value="0" oninput="updateEq(1, 'g', this.value)">
        </div>
        <div class="eq-band">
            <h4>Band 3 (Höhen)</h4>
            <label>Frequenz <span id="valEq2F" class="value-display">5000 Hz</span></label>
            <input type="range" id="eq2F" min="2000" max="20000" step="100" value="5000" oninput="updateEq(2, 'f', this.value)">
            <label>Q-Faktor <span id="valEq2Q" class="value-display">0.7</span></label>
            <input type="range" id="eq2Q" min="0.1" max="5.0" step="0.1" value="0.7" oninput="updateEq(2, 'q', this.value)">
            <label>Gain <span id="valEq2G" class="value-display">0.0 dB</span></label>
            <input type="range" id="eq2G" min="-15" max="15" step="0.5" value="0" oninput="updateEq(2, 'g', this.value)">
        </div>
    </div>

    <script>
        let raceActive = true; let filtersActive = true; let eqActive = false;
        function updateParam(param, value) {
            if(param === 'vol') document.getElementById('valVol').innerHTML = value + '%';
            if(param === 'delay') document.getElementById('valDelay').innerHTML = value + ' &micro;s';
            if(param === 'att') document.getElementById('valAtt').innerHTML = parseFloat(value).toFixed(1) + ' dB';
            if(param === 'center') document.getElementById('valCenter').innerHTML = (value/100).toFixed(2);
            fetch(`/api/update?${param}=${value}`);
        }
        function updateEq(band, param, value) {
            let labelId = 'valEq' + band + param.toUpperCase();
            if(param === 'f') document.getElementById(labelId).innerHTML = value + ' Hz';
            if(param === 'q') document.getElementById(labelId).innerHTML = parseFloat(value).toFixed(1);
            if(param === 'g') document.getElementById(labelId).innerHTML = parseFloat(value).toFixed(1) + ' dB';
            fetch(`/api/update?eq${band}${param}=${value}`);
        }
        function toggleRace() {
            raceActive = !raceActive;
            let btn = document.getElementById('btnRace');
            btn.innerText = raceActive ? "RACE: AN" : "RACE: AUS";
            btn.className = raceActive ? "" : "off";
            fetch(`/api/update?race=${raceActive ? 1 : 0}`);
        }
        function toggleFilters() {
            filtersActive = !filtersActive;
            let btn = document.getElementById('btnFilters');
            btn.innerText = filtersActive ? "Filter: AN" : "Filter: AUS";
            btn.className = filtersActive ? "" : "off";
            fetch(`/api/update?filters=${filtersActive ? 1 : 0}`);
        }
        function toggleEq() {
            eqActive = !eqActive;
            let btn = document.getElementById('btnEq');
            btn.innerText = eqActive ? "EQ: AN" : "EQ: AUS";
            btn.className = eqActive ? "" : "off";
            fetch(`/api/update?eq=${eqActive ? 1 : 0}`);
        }
        function calcGeometry() {
            let ds = parseFloat(document.getElementById('spkDist').value);
            let dl = parseFloat(document.getElementById('listDist').value);
            if(isNaN(ds) || isNaN(dl) || ds <= 0 || dl <= 0) return;
            let earDist = 17.5; let c = 34.32;      
            let d_ipsi = Math.sqrt(Math.pow(dl, 2) + Math.pow(ds/2 - earDist/2, 2));
            let d_contra = Math.sqrt(Math.pow(dl, 2) + Math.pow(ds/2 + earDist/2, 2));
            let delta_d = d_contra - d_ipsi;
            let idealDelay = Math.round((delta_d / c) * 1000); 
            let geoLoss = 20 * Math.log10(d_ipsi / d_contra);
            let idealAtt = geoLoss - (idealDelay * 0.015); 
            if (idealDelay > 1500) idealDelay = 1500; if (idealDelay < 20) idealDelay = 20;
            if (idealAtt < -30) idealAtt = -30; if (idealAtt > 0) idealAtt = 0;
            document.getElementById('delay').value = idealDelay;
            updateParam('delay', idealDelay);
            let attRounded = (Math.round(idealAtt * 10) / 10).toFixed(1);
            document.getElementById('att').value = attRounded;
            updateParam('att', attRounded);
        }
        const canvas = document.getElementById('analyzer');
        const ctx = canvas.getContext('2d');
        function drawSpectrum() {
            fetch('/api/spectrum')
                .then(r => r.json())
                .then(data => {
                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    let labelHeight = 20; let graphHeight = canvas.height - labelHeight;
                    let barWidth = (canvas.width / data.length) - 1.5; let x = 0;
                    let gradient = ctx.createLinearGradient(0, graphHeight, 0, 0);
                    gradient.addColorStop(0, '#007aff'); gradient.addColorStop(0.5, '#0a84ff'); gradient.addColorStop(1, '#ff3b30');
                    for(let i = 0; i < data.length; i++) {
                        let barHeight = data[i] * graphHeight;
                        ctx.fillStyle = gradient;
                        ctx.fillRect(x, graphHeight - barHeight, barWidth, barHeight);
                        x += barWidth + 1.5;
                    }
                    const minLog = Math.log10(20); const maxLog = Math.log10(22050); const logRange = maxLog - minLog;
                    const labels = [20, 50, 100, 500, 1000, 5000, 10000, 20000];
                    const labelTexts = ['20', '50', '100', '500', '1k', '5k', '10k', '20k'];
                    ctx.font = '11px -apple-system, BlinkMacSystemFont, sans-serif';
                    for (let i = 0; i < labels.length; i++) {
                        let pos = (Math.log10(labels[i]) - minLog) / logRange;
                        let labelX = pos * canvas.width;
                        ctx.fillStyle = 'rgba(255, 255, 255, 0.25)'; ctx.fillRect(labelX, 0, 1, graphHeight);
                        ctx.fillStyle = '#888';
                        if (i === 0) { ctx.textAlign = 'left'; labelX += 2; } 
                        else if (i === labels.length - 1) { ctx.textAlign = 'right'; labelX -= 2; } 
                        else { ctx.textAlign = 'center'; }
                        ctx.fillText(labelTexts[i], labelX, canvas.height - 4);
                    }
                    setTimeout(drawSpectrum, 50); 
                })
                .catch(e => { setTimeout(drawSpectrum, 1000); });
        }
        window.onload = () => {
            fetch('/api/status').then(r => r.json()).then(data => {
                document.getElementById('vol').value = data.volume * 100;
                document.getElementById('valVol').innerHTML = Math.round(data.volume * 100) + '%';
                document.getElementById('delay').value = data.delayUs;
                document.getElementById('valDelay').innerHTML = data.delayUs + ' &micro;s';
                document.getElementById('att').value = data.attenuationDb;
                document.getElementById('valAtt').innerHTML = data.attenuationDb.toFixed(1) + ' dB';
                document.getElementById('center').value = data.centerP * 100;
                document.getElementById('valCenter').innerHTML = data.centerP.toFixed(2);
                raceActive = data.raceActive; document.getElementById('btnRace').innerText = raceActive ? "RACE: AN" : "RACE: AUS"; document.getElementById('btnRace').className = raceActive ? "" : "off";
                filtersActive = data.filtersActive; document.getElementById('btnFilters').innerText = filtersActive ? "Filter: AN" : "Filter: AUS"; document.getElementById('btnFilters').className = filtersActive ? "" : "off";
                eqActive = data.eqActive; document.getElementById('btnEq').innerText = eqActive ? "EQ: AN" : "EQ: AUS"; document.getElementById('btnEq').className = eqActive ? "" : "off";
                for(let b=0; b<3; b++) {
                    document.getElementById('eq'+b+'F').value = data.eq[b].f; document.getElementById('valEq'+b+'F').innerHTML = data.eq[b].f + ' Hz';
                    document.getElementById('eq'+b+'Q').value = data.eq[b].q; document.getElementById('valEq'+b+'Q').innerHTML = data.eq[b].q.toFixed(1);
                    document.getElementById('eq'+b+'G').value = data.eq[b].g; document.getElementById('valEq'+b+'G').innerHTML = data.eq[b].g.toFixed(1) + ' dB';
                }
            });
            drawSpectrum();
        };
    </script>
</body>
</html>
)HTML";

int getKeyboardInput() {
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    return (ch != EOF) ? ch : -1;
}

void printStatus(float vol, float delayUs, float attDb, float center, bool raceActive, bool filtersActive, bool eqActive) {
    std::cout << "\r[Status] Vol: " << (int)(vol * 100) << "% | "
              << "RACE: " << (raceActive ? "AN" : "AUS") << " | "
              << "Filter: " << (filtersActive ? "AN" : "AUS") << " | "
              << "EQ: " << (eqActive ? "AN" : "AUS") << " | "
              << "Delay: " << delayUs << " us | "
              << "Att: " << attDb << " dB      " << std::flush;
}

// Reine Pipe-to-Pipe Verarbeitung (S32_LE zu S32_LE)
void audioProcessingLoop(RACEDspEngine* dspEngine, std::atomic<bool>& isRunning) {
    const char* inPipeName = "/tmp/oapw_in_stream";
    const char* outPipeName = "/tmp/oapw_stream";
    
    mkfifo(inPipeName, 0666); 
    mkfifo(outPipeName, 0666);
    chmod(inPipeName, 0666);
    chmod(outPipeName, 0666);
    
    int fdIn = -1;
    int fdOut = -1;
    
    // 2048 S32_LE (32-Bit) Werte pro Chunk
    std::vector<int32_t> intBuffer(2048);
    std::vector<audio_sample_t> doubleBuffer(2048);

    while (isRunning) {
        if (fdIn < 0) {
            fdIn = open(inPipeName, O_RDONLY | O_NONBLOCK);
            if (fdIn < 0) { usleep(50000); continue; }
        }
        if (fdOut < 0) {
            fdOut = open(outPipeName, O_WRONLY | O_NONBLOCK);
            if (fdOut < 0) { close(fdIn); fdIn = -1; usleep(100000); continue; }
        }

        ssize_t bytesRead = read(fdIn, intBuffer.data(), intBuffer.size() * sizeof(int32_t));
        
        if (bytesRead > 0) {
            size_t samplesRead = bytesRead / sizeof(int32_t);
            size_t validSamples = (samplesRead / 2) * 2; 
            
            if (validSamples > 0) {
                // 1. In Double wandeln (-1.0 bis 1.0)
                for (size_t i = 0; i < validSamples; ++i) {
                    doubleBuffer[i] = static_cast<audio_sample_t>(intBuffer[i]) / NORM_32; 
                }
                
                // 2. Vector für die Übergabe trimmen und Engine aufrufen
                std::vector<audio_sample_t> chunk(doubleBuffer.begin(), doubleBuffer.begin() + validSamples);
                dspEngine->processSamples(chunk);
                
                // 3. Zurück in 32-Bit Integer wandeln
                for (size_t i = 0; i < validSamples; ++i) {
                    intBuffer[i] = static_cast<int32_t>(chunk[i] * NORM_32);
                }
                
                // 4. In Ausgabe-Pipe schreiben
                write(fdOut, intBuffer.data(), validSamples * sizeof(int32_t));
            }
        } else if (bytesRead == 0) {
            close(fdIn); fdIn = -1;
            close(fdOut); fdOut = -1;
        } else {
            usleep(5000); 
        }
    }
    if (fdIn >= 0) close(fdIn);
    if (fdOut >= 0) close(fdOut);
}

void webServerLoop(RACEDspEngine* dspEngine, std::atomic<bool>& isRunning) {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(HTML_CONTENT, "text/html");
    });

    svr.Get("/api/status", [dspEngine](const httplib::Request&, httplib::Response& res) {
        std::string json = "{";
        json += "\"volume\":" + std::to_string(dspEngine->getVolume()) + ",";
        json += "\"delayUs\":" + std::to_string(dspEngine->getDelayUs()) + ",";
        json += "\"attenuationDb\":" + std::to_string(dspEngine->getAttenuationDb()) + ",";
        json += "\"centerP\":" + std::to_string(dspEngine->getCenterP()) + ",";
        json += "\"raceActive\":" + std::string(dspEngine->getRaceEnabled() ? "true" : "false") + ",";
        json += "\"filtersActive\":" + std::string(dspEngine->getFiltersEnabled() ? "true" : "false") + ",";
        json += "\"eqActive\":" + std::string(dspEngine->getEqEnabled() ? "true" : "false") + ",";
        json += "\"eq\":[";
        for(int b=0; b<3; b++) {
            json += "{\"f\":" + std::to_string(dspEngine->getEqFreq(b)) + 
                    ",\"q\":" + std::to_string(dspEngine->getEqQ(b)) + 
                    ",\"g\":" + std::to_string(dspEngine->getEqGain(b)) + "}";
            if(b < 2) json += ",";
        }
        json += "]}";
        res.set_content(json, "application/json");
    });

    svr.Get("/api/spectrum", [dspEngine](const httplib::Request&, httplib::Response& res) {
        std::vector<float> bands = dspEngine->getSpectrumBands();
        std::string json = "[";
        for (size_t i = 0; i < bands.size(); ++i) {
            json += std::to_string(bands[i]);
            if (i < bands.size() - 1) json += ",";
        }
        json += "]";
        res.set_content(json, "application/json");
    });

    svr.Get("/api/update", [dspEngine](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("vol")) dspEngine->setVolume(std::stof(req.get_param_value("vol")) / 100.0f);
        if (req.has_param("race")) dspEngine->setRaceEnabled(std::stoi(req.get_param_value("race")) != 0);
        if (req.has_param("filters")) dspEngine->setFiltersEnabled(std::stoi(req.get_param_value("filters")) != 0);
        if (req.has_param("eq")) dspEngine->setEqEnabled(std::stoi(req.get_param_value("eq")) != 0);

        if (req.has_param("delay") || req.has_param("att") || req.has_param("center")) {
            float delayUs = dspEngine->getDelayUs();
            float attDb = dspEngine->getAttenuationDb();
            float center = dspEngine->getCenterP();
            if (req.has_param("delay")) delayUs = std::stof(req.get_param_value("delay"));
            if (req.has_param("att")) attDb = std::stof(req.get_param_value("att"));
            if (req.has_param("center")) center = std::stof(req.get_param_value("center")) / 100.0f;
            dspEngine->setParameters(delayUs, attDb, center, dspEngine->getFreqLimit());
        }

        for(int b=0; b<3; b++) {
            std::string pf = "eq" + std::to_string(b) + "f";
            std::string pq = "eq" + std::to_string(b) + "q";
            std::string pg = "eq" + std::to_string(b) + "g";
            if (req.has_param(pf) || req.has_param(pq) || req.has_param(pg)) {
                float f = req.has_param(pf) ? std::stof(req.get_param_value(pf)) : dspEngine->getEqFreq(b);
                float q = req.has_param(pq) ? std::stof(req.get_param_value(pq)) : dspEngine->getEqQ(b);
                float g = req.has_param(pg) ? std::stof(req.get_param_value(pg)) : dspEngine->getEqGain(b);
                dspEngine->setEqBand(b, f, q, g);
            }
        }
        res.set_content("OK", "text/plain");
    });

    std::cout << "\n[Web] Web-Interface gestartet auf Port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}

int main(int argc, char** argv) {
    std::atomic<bool> isRunning(true);
    
    RACEDspEngine dspEngine(68.0, -2.3, 0.1, true);
    dspEngine.setVolume(0.5);

    std::thread audioThread(audioProcessingLoop, &dspEngine, std::ref(isRunning));
    std::thread webThread(webServerLoop, &dspEngine, std::ref(isRunning));
    webThread.detach();

    std::cout << "\n=== Steuerung (Live) ===" << std::endl;
    std::cout << " DSP-Engine laeuft (192kHz/64-Bit)" << std::endl;
    std::cout << " I/O via Pipes (/tmp/oapw_in_stream -> /tmp/oapw_stream)" << std::endl;
    std::cout << " [q] : Beenden" << std::endl;
    std::cout << "========================\n" << std::endl;

    while (isRunning) {
        int key = getKeyboardInput();
        if (key == 'q') isRunning = false;
        
        printStatus(dspEngine.getVolume(), dspEngine.getDelayUs(), dspEngine.getAttenuationDb(), dspEngine.getCenterP(), dspEngine.getRaceEnabled(), dspEngine.getFiltersEnabled(), dspEngine.getEqEnabled());
        usleep(500000); 
    }

    std::cout << "\n\nFahre Engine herunter..." << std::endl;
    if (audioThread.joinable()) audioThread.join();
    
    return 0;
}
