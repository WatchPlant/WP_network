import base64
import datetime
import json

import dash
import dash_bootstrap_components as dbc
import paho.mqtt.client as mqtt
from dash import dcc, html
from dash.dependencies import Input, Output, State

app = dash.Dash(__name__)

# MQTT Configurations
mqtt_broker = "eu1.cloud.thethings.network"
mqtt_port = 8883
mqtt_topic = "v3/larics-lora-test@ttn/devices/eui-b36062ec000058be/down/replace"
USER = 'larics-lora-test@ttn'
PASS = 'NNSXS.F6M37XAEBLJ5K4TDZWOZII3JQUPU3JCHDWWC5XA.K3R74KDUAZIF7BUDEHJPD2F6VPORLAULJSYYFHQSNQRYJMISCLNA'

# Layout of the application
app.layout = dbc.Container(
    [
        dbc.Row(
            dbc.Col(html.H1("Request plant data"), width={"size": 6, "offset": 3}),
            className="mt-5 mb-4"
        ),
        dbc.Row(
            dbc.Col(dbc.Input(id='mobile-input', type='text', placeholder='Enter mobile phone number', className="mb-3")),
            className="mb-3"
        ),
        dbc.Row(
            dbc.Col(dbc.Input(id='plant-input', type='text', placeholder='Enter plant ID', className="mb-3")),
            className="mb-3"
        ),
        dbc.Row(
            dbc.Col(dbc.Button('Submit', id='submit-btn', n_clicks=0, color="primary", className="mb-3")),
            className="mb-3"
        ),
        dbc.Row(
            dbc.Col(html.Div(id='output-message', className="mb-3")),
            className="mb-3"
        )
    ]
)

# MQTT Client Setup
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(USER, PASS)
mqtt_client.tls_set()
mqtt_client.connect("eu1.cloud.thethings.network", 8883, 60)
mqtt_client.loop_start()

# Callback to handle button click event
@app.callback(
    Output('output-message', 'children'),
    Input('submit-btn', 'n_clicks'),
    State('mobile-input', 'value'),
    State('plant-input', 'value')
)
def on_submit(n_clicks, mobile_input, plant_input):
    if n_clicks > 0:
        # 1st byte is message type id.
        # 2nd byte is the plant number so it's easier to parse on the microcontroller.
        # 3rd+ bytes are the mobile phone number.
        payload = bytearray([2, int(plant_input)]) + mobile_input.encode()
                                                
        value = {
            "downlinks": [{
                "f_port": 15,  # 1-233 allowed
                "frm_payload": base64.b64encode(payload).decode(),
                "priority": "NORMAL"
            }]
            }
        value = json.dumps(value)

        # Publish to MQTT
        mqtt_client.publish(mqtt_topic, value)

        return dbc.Alert(f'[{datetime.datetime.now()}] Successfully sent to MQTT.\n{value}', color="success")

    return ''


if __name__ == '__main__':
    app.run_server(debug=True)