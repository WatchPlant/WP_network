import base64
import datetime
import json
import os
import re

import dash
import dash_bootstrap_components as dbc
import paho.mqtt.client as mqtt
from dash import dcc, html
from dash.dependencies import Input, Output, State

TTN_USER = os.environ["TTN_USER"]
TTN_PASS = os.environ["TTN_PASS"]


# Use a different theme for a more professional look
app = dash.Dash(__name__, external_stylesheets=[dbc.themes.MINTY])
server = app.server

# MQTT Configurations
mqtt_broker = "eu1.cloud.thethings.network"
mqtt_port = 8883
mqtt_topic = f"v3/{TTN_USER}/devices/eui-b36062ec000058be/down/replace"

# Regular expression for European phone numbers
european_phone_regex = r"^\+(?:[0-9] ?){6,14}[0-9]$"

# Function to validate phone number
def is_valid_phone_number(phone_number):
    return re.match(european_phone_regex, phone_number) is not None

# Layout of the application
app.layout = dbc.Container(
    [
        dbc.Navbar(
            dbc.Container(
                dbc.NavbarBrand("WatchPlant review demo", className="mx-auto", style={'fontSize': '52px'})
            ),
            color="dark",
            dark=True,
            style={'height': '70px'}  # Adjust the height of the Navbar
        ),
        dbc.Row(
            dbc.Col(html.H1("Request Plant Data", className="text-center"), width={"size": 6}),
            className="mt-5 mb-4 justify-content-center"
        ),
        dbc.Row(
            dbc.Col(dbc.Input(id='mobile-input', type='text', placeholder='Enter Mobile Phone Number', className="mb-3"),
                    width=4, className="mx-auto"),
            className="mb-6 justify-content-center"
        ),
        dbc.Row(
            dbc.Col(dbc.Input(id='plant-input', type='text', placeholder='Enter Plant ID', className="mb-3"), width=4,
                    className="mx-auto"),
            className="mb-6 justify-content-center"
        ),
        dbc.Row(
            dbc.Col(dbc.Button('Submit', id='submit-btn', n_clicks=0, color="primary", className="mb-3"), width=4,
                    className="mx-auto"),
            className="mb-6 justify-content-center"
        ),
        dbc.Row(
            dbc.Col(html.Div(id='output-message', className="mb-3"), width=6, className="mx-auto"),
            className="mb-6 justify-content-center"
        ),
         dbc.Row(
            dbc.Col(html.Img(src='/assets/watchplant-removebg-preview.png', height='200px', style={'position': 'fixed', 'bottom': '0', 'right': '0'}),
                    width=12),
            className="mb-6"
        )
    ],
    fluid=True,  # Use a fluid layout for better responsiveness
    style={'background-color': '#f2f2f2', 'min-height': '100vh'}  # Light gray background color
)

# MQTT Client Setup
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(TTN_USER, TTN_PASS)
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
        # TODO: Check
        if not is_valid_phone_number(mobile_input):
            return dbc.Alert('Invalid phone number. Please enter a valid European phone number.', color="danger")

        
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
    app.run(host='0.0.0.0', debug=True)
