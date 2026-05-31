import asyncio
import json
import websockets
from datetime import datetime

CPP_HOST = "127.0.0.1"
CPP_PORT = 9090

clients = {}

def now_time():
    return datetime.now().strftime("%H:%M")

def parse_history_line(line):
    # Format: [2026-05-31 06:29:42] hari: hello
    try:
        t_end = line.index(']')
        timestamp = line[1:t_end]
        rest = line[t_end+2:]
        colon = rest.index(': ')
        username = rest[:colon]
        message = rest[colon+2:]
        time = timestamp[11:16]  # extract HH:MM
        return {"username": username, "text": message, "time": time}
    except:
        return None

async def cpp_receiver(websocket, reader, username):
    buffer = ""
    in_history = False
    history_lines = []

    try:
        while True:
            data = await reader.read(4096)
            if not data:
                break
            buffer += data.decode(errors='replace')

            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                line = line.strip()
                if not line:
                    continue

                if line == "HISTORY_START":
                    in_history = True
                    history_lines = []
                    continue

                if line == "HISTORY_END":
                    in_history = False
                    for hl in history_lines:
                        parsed = parse_history_line(hl)
                        if parsed:
                            await websocket.send(json.dumps({
                                "type": "history",
                                "messages": [parsed]
                            }))
                    continue

                if in_history:
                    history_lines.append(line)
                    continue

                if ": " in line:
                    parts = line.split(": ", 1)
                    sender = parts[0]
                    msg = parts[1]
                    print(f"sender='{sender}' username='{username}' match={sender==username}")
                    if sender == username:
                        continue
                    await websocket.send(json.dumps({
                        "type": "message",
                        "username": sender,
                        "text": msg,
                        "time": now_time()
                    }))

    except Exception as e:
        print(f"cpp_receiver error: {e}")

async def handler(websocket):
    reader, writer = await asyncio.open_connection(CPP_HOST, CPP_PORT)
    username = None

    try:
        async for raw in websocket:
            msg = json.loads(raw)

            if msg["type"] == "join":
                username = msg["username"]
                clients[websocket] = username

                # Send username to C++ server
                writer.write((username + "\0").encode())
                await writer.drain()

                # Start receiving from C++ server
                asyncio.create_task(cpp_receiver(websocket, reader, username))

            elif msg["type"] == "message" and username:
                text = msg["text"]

               
                # Forward to C++ server (it will broadcast to others)
                writer.write((text + "\0").encode())
                await writer.drain()

    except websockets.exceptions.ConnectionClosed:
        pass
    except Exception as e:
        print(f"handler error: {e}")
    finally:
        if websocket in clients:
            del clients[websocket]
        writer.close()

async def main():
    print("Bridge running on ws://localhost:8765")
    print("Connecting to C++ server on tcp://localhost:9090")
    async with websockets.serve(handler, "localhost", 8765):
        await asyncio.Future()

asyncio.run(main())