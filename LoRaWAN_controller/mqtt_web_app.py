import dash
from dash import dcc, html
import dash_bootstrap_components as dbc
from dash.dependencies import Input, Output, State
import base64
import paho.mqtt.client as mqtt
import json

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
        command = bytes.fromhex('02')
        data = f"{plant_input},{mobile_input}"

        value = {
            "downlinks": [{
                "f_port": 15,  # 1-233 allowed
                "frm_payload": base64.b64encode(command + data.encode()).decode(),
                "priority": "NORMAL"
            }]
            }
        value = json.dumps(value)

        # Publish to MQTT
        mqtt_client.publish(mqtt_topic, value)

        return dbc.Alert(f'Successfully sent to MQTT.\n{value}', color="success")

    return ''


if __name__ == '__main__':
    app.run_server(debug=True)