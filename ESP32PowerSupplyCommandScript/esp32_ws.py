#!/usr/bin/env python3
import asyncio
import websockets
import json

# Replace with the IP address of your ESP32.
# If your ESP32 is at 192.168.1.45, for example, use "ws://192.168.1.45:81"
esp32_ip = "192.168.0.156"
ws_url = f"ws://{esp32_ip}:7777"

async def set_value(value: int):
    """
    Sends the 'setValue' command with a given integer value (0-255) to the ESP32 WebSocket server.
    """
    async with websockets.connect(ws_url) as websocket:
        # Construct the JSON command
        command = {
            "command": "setValue",
            "value": value
        }
        # Send command to the server
        await websocket.send(json.dumps(command))
        print(f"Sent setValue command with value={value}")

        # Wait for the server's response
        response = await websocket.recv()
        print("Received response:", response)

async def get_status():
    """
    Sends the 'getStatus' command to the ESP32 WebSocket server and prints the response.
    """
    async with websockets.connect(ws_url) as websocket:
        # Construct the JSON command
        command = {
            "command": "getStatus"
        }
        # Send command to the server
        await websocket.send(json.dumps(command))
        print("Sent getStatus command")

        # Wait for the server's response
        response = await websocket.recv()
        print("Received response:", response)

async def enable(state: bool):
    """
    Sends the 'enable' command to set pin 15 HIGH (True) or LOW (False) on the ESP32 WebSocket server.
    """
    async with websockets.connect(ws_url) as websocket:
        # Construct the JSON command
        command = {
            "command": "enable",
            "value": state  # Convert boolean to lowercase string
        }
        # Send command to the server
        await websocket.send(json.dumps(command))
        print(f"Sent enable command with value={state}")

        # Wait for the server's response
        response = await websocket.recv()
        print("Received response:", response)

def main():
    # Examples of how you might use these functions
    # Uncomment the ones you want to test
    asyncio.run(set_value(128))  # Set value to 128
    asyncio.run(get_status())    # Request status from the server
    asyncio.run(enable(True))    # Enable (set pin 15 HIGH)
    asyncio.run(enable(False))   # Disable (set pin 15 LOW)

if __name__ == "__main__":
    main()
