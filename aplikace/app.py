from flask import Flask, render_template, jsonify, request
import paho.mqtt.client as mqtt

app = Flask(__name__, static_url_path='/static', static_folder='static')

# MQTT konfigurace
MQTT_BROKER = "10.0.1.41"
MQTT_PORT = 1884
MQTT_USER = "mqttuser"
MQTT_PASSWORD = "12345"

MQTT_TOPICS = [
    "sensor/temperature",
    "sensor/humidity",
    "senseor/temperaturebmp",
    "sensor/pressure",
    "sensor/water_temperature",
    "sensor/ph",
    "sensor/tds",
    "control/led",
    "control/led/schedule",
    "control/outfan",
    "control/outfan/settings",
    "control/infan",
    "control/infan/interval",
    "control/pump",
    "control/ph",
    "control/tds"
]

sensor_data = {
    "sensor/temperature": "N/A",
    "sensor/humidity": "N/A",
    "sensor/temperaturebmp": "N/A",
    "sensor/pressure": "N/A",
    "sensor/water_temperature": "N/A",
    "sensor/ph": "N/A",
    "sensor/tds": "N/A",
    "control/led": "VYPNUTO",
    "control/outfan": "VYPNUTO",
    "control/infan": "VYPNUTO",
    "control/pump": "VYPNUTO",
    "control/led/schedule": "none",
    "control/outfan/settings": "none",
    "control/infan/interval": "none",
    "control/ph": "none",
    "control/tds": "none"
}

# MQTT callbacks
def on_connect(client, userdata, flags, rc):
    print("Připojeno k MQTT brokeru s kódem: " + str(rc))
    for topic in MQTT_TOPICS:
        client.subscribe(topic)
        print("Přihlášuji se k tématu: " + topic)

def on_message(client, userdata, msg):
    print(f"Přijato {msg.topic}: {sensor_data[msg.topic]}")
    sensor_data[msg.topic] = msg.payload.decode("utf-8")
    print(f"Aktualizované téma {msg.topic}: {sensor_data[msg.topic]}")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

@app.route('/')
def index():
    return render_template("index.html")

# získání dat ze senzorů
@app.route('/data')
def get_data():
    mapped_data = {
        "temperature": sensor_data["sensor/temperature"],
        "temperaturebmp": sensor_data["sensor/temperaturebmp"],
        "humidity": sensor_data["sensor/humidity"],
        "pressure": sensor_data["sensor/pressure"],
        "water_temperature": sensor_data["sensor/water_temperature"],
        "ph": sensor_data["sensor/ph"],
        "tds": sensor_data["sensor/tds"]
    }
    return jsonify(mapped_data)

# ovládání zařízení
@app.route('/control', methods=['POST'])
def control_device():
    data = request.get_json()
    device = data.get("device")
    state = data.get("state")

    if device and state:
        topic = f"control/{device}"
        print(f"MQTT SEND → {topic}: {state}")
        mqtt_client.publish(topic, state, retain=True)
        return jsonify({"status": "success", "device": device, "state": state}), 200
    else:
        return jsonify({"status": "error", "message": "Neplatný request"}), 400

# získání stavu ovládání
@app.route('/control/state')
def get_control_state():
    control_state = {
        "led": sensor_data.get("control/led", "VYPNUTO"),
        "schedule": sensor_data.get("control/led/schedule", "none"),
        "outfan": sensor_data.get("control/outfan", "VYPNUTO"),
        "outfan_setting": sensor_data.get("control/outfan/settings", "none"),
        "pump": sensor_data.get("control/pump", "VYPNUTO"),
        "infan": sensor_data.get("control/infan", "VYPNUTO"),
        "infan_interval": sensor_data.get("control/infan/interval", "none"),
        "ph": sensor_data.get("control/ph", "none"),
        "tds": sensor_data.get("control/tds", "none")
    }

    print("--> aktuální stav:", control_state)
    return jsonify(control_state)


# LED OVLÁDÁNÍ
# plánování LED osvětlení
@app.route('/schedule', methods=['POST'])
def set_schedule():
    data = request.get_json()
    start_time = data.get("start_time")
    end_time = data.get("end_time")
    if start_time and end_time:
        schedule_payload = f"{start_time}-{end_time}"
        print(f"Nastavuji plán LED: {schedule_payload}")
        mqtt_client.publish("control/led/schedule", schedule_payload, retain=True)
        sensor_data["control/led/schedule"] = schedule_payload
        return jsonify({"status": "success", "schedule": schedule_payload}), 200
    else:
        return jsonify({"status": "error", "message": "Neplatný vstup pro plán LED osvětlení."}), 400

# smazání plánu LED
@app.route('/schedule/delete', methods=['POST'])
def delete_schedule():
    print("Mazání plánu LED osvětlení - vypínám automatické řízení LED osvětlení.")
    mqtt_client.publish("control/led/schedule", "none", retain=True)
    sensor_data["control/led/schedule"] = "none"
    return jsonify({"status": "success", "schedule": "none"}), 200


# VĚTRÁK VNĚJŠÍ
# získání nastavení větráku
@app.route('/outfan/settings/state', methods=['GET'])
def get_outfan_settings_state():
    outfan_setting = sensor_data.get("control/outfan/settings", "none")
    return jsonify({"status": "success", "outfan_setting": outfan_setting}), 200


# nastavení mezních hodnot pro větrák out
@app.route('/outfan/settings', methods=['POST'])
def set_outfan_settings():
    data = request.get_json()
    temperature = data.get("temperature")
    humidity = data.get("humidity")
    outfan_setting_parts = []

    if temperature not in [None, "", "none"]:
        sensor_data["settings/outfan/temperature"] = str(temperature)
        outfan_setting_parts.append(f"T={temperature}°C")
        print(f"Nová mez teploty vnějšího větráku: {temperature}°C")
    else:
        sensor_data["settings/outfan/temperature"] = "none"
        print("Mez teploty = deaktivovaná")

    if humidity not in [None, "", "none"]:
        sensor_data["settings/outfan/humidity"] = str(humidity)
        outfan_setting_parts.append(f"H={humidity}%")
        print(f"Nová mez vlhkosti vnějšího větráku: {humidity}%")
    else:
        sensor_data["settings/outfan/humidity"] = "none"
        print("Mez vlhkosti = deaktivovaná")

    if outfan_setting_parts:
        outfan_payload = "&".join(outfan_setting_parts)
    else:
        outfan_payload = "none"

    sensor_data["control/outfan/settings"] = outfan_payload
    mqtt_client.publish("control/outfan/settings", outfan_payload, retain=True)
    print(f"odesláno --> control/outfan/settings: {outfan_payload}")

    return jsonify({
        "status": "success",
        "outfan_setting": outfan_payload
    }), 200


# smazání mezních hodnot pro větrák out
@app.route('/outfan/settings/delete', methods=['POST'])
def delete_outfan_settings():
    sensor_data["settings/outfan/temperature"] = "none"
    sensor_data["settings/outfan/humidity"] = "none"
    sensor_data["control/outfan/settings"] = "none"

    mqtt_client.publish("control/outfan/settings", "none", retain=True)
    print("Mezní hodnoty vnějšího větráku byly smazány - vypínám automatické řízení vnějšího větráku.")

    return jsonify({"status": "success", "outfan_setting": "none"}), 200


# VĚTRÁK VNITŘNÍ
# nastavení intervalu pro větrák in
@app.route('/infan/interval', methods=['POST'])
def set_infan_interval():
    data = request.get_json()
    interval_minutes = data.get("interval_minutes")
    if interval_minutes:
        mqtt_client.publish("control/infan/interval", str(interval_minutes), retain=True)
        sensor_data["control/infan/interval"] = str(interval_minutes)
        print(f"Nový interval vnitřního větráku = {interval_minutes} minut/hod.")
        return jsonify({"status": "success", "interval": interval_minutes}), 200
    return jsonify({"status": "error"}), 400

# smazání intervalu pro větrák in
@app.route('/infan/interval/delete', methods=['POST'])
def delete_infan_interval():
    mqtt_client.publish("control/infan/interval", "none", retain=True)
    sensor_data["control/infan/interval"] = "none"
    print("Interval vnitřního větrák = deaktivovaný - vypínám automatické řízení vnitřního větráku.")
    return jsonify({"status": "success"}), 200

# PH
# nastavení pH ovládání
@app.route('/ph/settings', methods=['POST'])
def set_ph_settings():
    data = request.get_json()
    ph_max = data.get("ph_max")
    ph_min = data.get("ph_min")

    if ph_max is None or ph_min is None:
        return jsonify({"status": "error", "message": "Chybí hodnoty pH"}), 400

    # sestavení payloadu, "ph_min&ph_max"
    payload = f"{ph_min}&{ph_max}"
    mqtt_client.publish("control/ph", payload, retain=True)
    sensor_data["control/ph"] = payload
    print(f"pH nastavení = {payload}")
    return jsonify({"status": "success", "ph_setting": payload}), 200

# smazání pH ovládání
@app.route('/ph/settings/delete', methods=['POST'])
def delete_ph_settings():
    mqtt_client.publish("control/ph", "none", retain=True)
    sensor_data["control/ph"] = "none"
    print("pH nastavení = deaktivované - vypínám automatické řízení pH.")
    return jsonify({"status": "success", "ph_setting": "none"}), 200

# TDS
# nastavení minimální TDS hodnoty
@app.route('/tds/settings', methods=['POST'])
def set_tds_settings():
    data = request.get_json()
    tds_min = data.get("tds_min_value")
    if tds_min is None or tds_min == "":
         return jsonify({"status": "error", "message": "Chybí minimální TDS hodnota"}), 400
    mqtt_client.publish("control/tds", str(tds_min), retain=True)
    sensor_data["control/tds"] = str(tds_min)
    print(f"Nová minimální TDS = {tds_min}")
    return jsonify({"status": "success", "tds_setting": tds_min}), 200

# smazání minimální TDS hodnoty
@app.route('/tds/settings/delete', methods=['POST'])
def delete_tds_settings():
    mqtt_client.publish("control/tds", "none", retain=True)
    sensor_data["control/tds"] = "none"
    print("Minimální TDS = deaktivovaná - vypínám automatické řízení TDS.")
    return jsonify({"status": "success", "tds_setting": "none"}), 200


# spuštění: flask run --host=0.0.0.0 --port=5050
if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5050, debug=True)
