const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<body>
    <p>Select an action:</p>
    <input type="radio" id="ap" name="action" value="AP">
    <label for="AP">Scan for AP</label><br>
    <input type="radio" id="client" name="action" value="CLIENT">
    <label for="AP">Scan for connected clients</label><br>
    <input type="radio" id="spoof" name="action" value="SPOOF">
    <label for="AP">Deauth and spoof access point</label><br>
</body>
</html>
)rawliteral";